/*
 * Copyright (c) 2013, M.Naruoka (fenrir)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the naruoka.org nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if defined(_MSC_VER) && _MSC_VER >= 1400
#define _USE_MATH_DEFINES
//#define sprintf sprintf_s
typedef __int64 int64_t;
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <exception>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#define IS_LITTLE_ENDIAN 1
#include "SylphideStream.h"
#include "SylphideProcessor.h"

typedef double float_sylph_t;
#include "analyze_common.h"

using namespace std;

struct Options : public GlobalOptions {
  bool page_A;
  bool page_G;
  bool page_F;
  bool page_P;
  bool page_M;
  bool page_N;
  bool page_other;
  int page_P_mode, page_F_mode, page_M_mode;
  int debug_level;
  
  Options() 
      : GlobalOptions(),
      page_A(false), page_G(false), page_F(false), 
      page_P(false), page_M(false), page_N(false),
      page_other(false),
      page_P_mode(5),
      page_F_mode(3),
      page_M_mode(0),
      debug_level(0) {}
  ~Options(){}
  
  /**
   * �R�}���h�ɗ^����ꂽ�ݒ��ǂ݉���
   * 
   * @param spec �R�}���h
   * @return (bool) ��ǂɃq�b�g�����ꍇ��true�A�����Ȃ����false
   */
  bool check_spec(char *spec){
    char c;
    if(strstr(spec, "--page_P_mode=") == spec){
      char *value(spec + strlen("--page_P_mode="));
      page_P_mode = atoi(value);
      cerr << "page_P_mode: " << page_P_mode << endl;
      return true;
    }
    if(strstr(spec, "--page_F_mode=") == spec){
      // page_F_mode = [bit0 => ���͂�\��, bit1 => �o�͂�\��]
      char *value(spec + strlen("--page_F_mode="));
      page_F_mode = atoi(value);
      cerr << "page_F_mode: " << page_F_mode << endl;
      return true;
    }
    if(strstr(spec, "--page_M_mode=") == spec){
      char *value(spec + strlen("--page_M_mode="));
      page_M_mode = atoi(value);
      cerr << "page_M_mode: " << page_M_mode << endl;
      return true;
    }
    if(strstr(spec, "--page_other=") == spec){
      char *value(spec + strlen("--page_other="));
      page_other = (strcmp(value, "on") == 0);
      cerr << "page_other: " << (page_other ? "on" : "off") << endl;
      return true;
    }
    if((strstr(spec, "--page=") == spec) 
        && (c = *(spec + strlen("--page=")))){
      switch(c){
        case 'A': page_A = true; break;
        case 'G': page_G = true; break;
        case 'F': page_F = true; break;
        case 'M': page_M = true; break;
        case 'N': page_N = true; break;
        case 'P': page_P = true; break;
        default: return false;
      }
      return true;
    }
    // �݊����̂��߁Ain_sylphide�Ɠ�����direct_sylphide���c���Ă���
    if(strstr(spec, "--direct_sylphide=") == spec){
      char *value(spec + strlen("--direct_sylphide="));
      in_sylphide = (strcmp(value, "on") == 0);
      cerr << "direct_sylphide: " << (in_sylphide ? "on" : "off") << endl;
      return true;
    }
    if(strstr(spec, "--debug=") == spec){
      char *value(spec + strlen("--debug="));
      debug_level = atoi(value);
      cerr << "debug_level: " << debug_level << endl;
      return true;
    }
    return GlobalOptions::check_spec(spec);
  }
} options;

class StreamProcessor : public SylphideProcessor<float_sylph_t> {
  protected:
    int invoked;
    typedef SylphideProcessor<float_sylph_t> super_t;
  
  public:
    /**
     * A�y�[�W(AD�ϊ�����)�̏����p�֐�
     * A�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
     * 
     * @param obsrever A�y�[�W�̃I�u�U�[�o�[
     */
    struct HandlerA {
      int count;
      HandlerA() : count(0) {}
      void operator()(const super_t::A_Observer_t &observer){
        if(!observer.validate()){return;}
        
        float_sylph_t current(observer.fetch_ITOW());
        if(!options.is_time_in_range(current)){return;}
        
        options.out() 
            << (count++) << ","
            << current << ",";
        
        A_Observer_t::values_t values(observer.fetch_values());
        for(int i(0); i < 8; i++){
          options.out() << values.values[i] << ",";
        }
        options.out() << values.temperature << endl;
      }
    } handler_A;
    
    /**
     * G�y�[�W(u-blox��GPS)�̏����p�֐�
     * G�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
     * {class, id} = {0x01, 0x02}�̂Ƃ��ʒu��񂪓�����
     * {class, id} = {0x01, 0x12}�̂Ƃ����x��񂪓�����
     * 
     * @param obsrever G�y�[�W�̃I�u�U�[�o�[
     */
    struct HandlerG {
      unsigned int itow_ms_0x0102, itow_ms_0x0112;
      super_t::G_Observer_t::position_t position;
      super_t::G_Observer_t::position_acc_t position_acc;
      super_t::G_Observer_t::velocity_t velocity;
      super_t::G_Observer_t::velocity_acc_t velocity_acc;
      HandlerG() 
          : itow_ms_0x0102(0), itow_ms_0x0112(0),
          position(0, 0, 0), position_acc(0, 0),
          velocity(0, 0, 0), velocity_acc(0) {
        
      }
      ~HandlerG(){}
      
      void operator()(const super_t::G_Observer_t &observer){
        if(!observer.validate()){return;}
        
        bool change_itow(false);
        super_t::G_Observer_t::packet_type_t
            packet_type(observer.packet_type());
        switch(packet_type.mclass){
          case 0x01: {
            switch(packet_type.mid){
              case 0x02: {
                position = observer.fetch_position();
                position_acc = observer.fetch_position_acc();
                
                itow_ms_0x0102 = observer.fetch_ITOW_ms();
                change_itow = true;
                
                break;
              }
              case 0x12: {
                velocity = observer.fetch_velocity();
                velocity_acc = observer.fetch_velocity_acc();
                
                itow_ms_0x0112 = observer.fetch_ITOW_ms();
                change_itow = true;
                
                break;
              }
            }
            break;
          }
          default:
            break;
        }
        
        if(change_itow
            && (itow_ms_0x0102 == itow_ms_0x0112)){
          
          float_sylph_t current(1E-3 * itow_ms_0x0102);
          if(!options.is_time_in_range(current)){return;}
          
          options.out() << current << ", "
              << position.latitude << ", "
              << position.longitude << ", "
              << position.altitude << ", "
              << position_acc.horizontal << ", "
              << position_acc.vertical << ", "
              << velocity.north << ", "
              << velocity.east << ", "
              << velocity.down << ", "
              << velocity_acc.acc << endl;
        }
      }
    } handler_G;
    
    /**
     * F�y�[�W(FPGA�T�u�V�X�e������̏��)�̏����p�֐�
     * F�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
     * 
     * @param obsrever F�y�[�W�̃I�u�U�[�o�[
     */
    struct HandlerF {
      int count;
      HandlerF() : count(0) {}
      void operator()(const F_Observer_t &observer){
        if(!observer.validate()){return;}
        
        float_sylph_t current(observer.fetch_ITOW());
        if(!options.is_time_in_range(current)){return;}
        
        options.out() << (count++)
             << ", " << current;
        
        F_Observer_t::values_t values(observer.fetch_values());
        for(int i = 0; i < 8; i++){
          //if(values.servo_in[i] < 1000){values.servo_in[i] += 1000;}
          if(options.page_F_mode & 0x01){ // �r�b�g0�����͂�\��
            options.out() << ", " << values.servo_in[i];
          }
          if(options.page_F_mode & 0x02){ // �r�b�g1���o�͂�\��
            options.out() << ", " << values.servo_out[i];
          }
        }
        options.out() << endl;
      }
    } handler_F;
    
    
    struct HandlerP {
      HandlerP() {}
      
      void ms5611_convert(
          const Int32 &d1, const Int32 &d2,
          Int32 &pressure, Int32 &temperature,
          const Uint16 (&coef)[6]) const {
        Int32 dT(d2 - (coef[4] << 8));
        temperature = (Int32)2000 + (((int64_t)dT * coef[5]) >> 23);

        int64_t off(((int64_t)coef[1] << 16) + (((int64_t)coef[3] * dT) >> 7));
        int64_t sens(((int64_t)coef[0] << 15) + (((int64_t)coef[2] * dT) >> 8));

        // Figure 3
        if(temperature < 2000){
          Int32 t2(((int64_t)dT * dT) << 31);
          Int32 dT2(temperature - 2000), dT2_2(dT2 * dT2);
          Int32 off2((dT2_2 * 5) >> 1);
          Int32 sens2((dT2_2 * 5) >> 2);
          if(temperature < -1500){
            Int32 dT3(temperature + 1500), dT3_2(dT3 * dT3);
            off2 += dT3_2 * 7;
            sens2 += (dT3_2 * 11) >> 1;
          }
          temperature -= t2;
          off -= off2;
          sens -= sens2;
        }

        pressure = (Int32)(((((int64_t)d1 * sens) >> 21) - off) >> 15);
      }

      /**
       * P�y�[�W(�G�A�f�[�^�Z���T����̏��)�̏����p�֐�
       * P�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
       * 
       * @param obsrever F�y�[�W�̃I�u�U�[�o�[
       */
      void operator()(const P_Observer_t &observer){
        if(!observer.validate()){return;}
        
        float_sylph_t current(observer.fetch_ITOW());
        if(!options.is_time_in_range(current)){return;}
        
        switch(options.page_P_mode){
          case 5: { // MS5611 with coefficients
            Uint16 coef[6];
            char buf[sizeof(Uint16)];
            for(int i(0); i < sizeof(coef) / sizeof(coef[0]); i++){
              observer.inspect(&buf[0], sizeof(buf), 19 + (sizeof(buf) * i));
              coef[i] = be_char2_2_num<Uint16>(*buf);
            }

            for(int i(0); i < 2; i++){
              options.out() << current << ", " << (i - 1) << ", ";
              char buf[4][2];
              buf[0][0] = buf[0][1] = 0;
              observer.inspect(&buf[1][0], 3, 7 + 6 * i);
              observer.inspect(&buf[1][1], 3, 10 + 6 * i);
              Uint32
                  d1(be_char4_2_num<Uint32>(buf[0][0])),
                  d2(be_char4_2_num<Uint32>(buf[0][1]));
              Int32 pressure, temperature;
              ms5611_convert(d1, d2, pressure, temperature, coef);
              options.out()
                  << pressure << ", "
                  << temperature << endl;
            }
            break;
          }
        }
      }
    } handler_P;
    
    /**
     * M�y�[�W(���C�Z���T����̏��)�̏����p�֐�
     * M�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
     * 
     * @param obsrever M�y�[�W�̃I�u�U�[�o�[
     */
    struct HandlerM {
      void operator()(const M_Observer_t &observer){
        if(!observer.validate()){return;}
        
        float_sylph_t current(observer.fetch_ITOW());
        if(!options.is_time_in_range(current)){return;}
        
        M_Observer_t::values_t values(observer.fetch_values());

        switch(options.page_M_mode){
          case 1: // -atan2(y, x)��������[deg]��\��
            for(int i(0); i < 4; i++){
              options.out() << current << ", "
                   << (i - 3) << ", "
                   << rad2deg(-atan2((double)values.y[i], (double)values.x[i])) << endl;
            }
            break;
          default:
            for(int i(0); i < 4; i++){
              options.out() << current << ", "
                   << (i - 3) << ", "
                   << values.x[i] << ", "
                   << values.y[i] << ", "
                   << values.z[i] << endl;
            }
        }
      }
    } handler_M;
    
    /**
     * N�y�[�W(�q�@���)�̏����p�֐�
     * N�y�[�W�̓��e����������validate�Ŋm�F������A���������s�����ƁB
     * 
     * @param obsrever N�y�[�W�̃I�u�U�[�o�[
     */
    struct HandlerN {
      void operator()(const N_Observer_t &observer){
        if(!observer.validate()){return;}
        
        float_sylph_t current(observer.fetch_ITOW());
        if(!options.is_time_in_range(current)){return;}
        
        switch(observer.kind()){
          case 0: {
            N_Observer_t::navdata_t values(observer.fetch_navdata());
            
            options.out() << values.itow << ", "
                << values.longitude << ", "
                << values.latitude << ", "
                << values.altitude << ", "
                << values.v_north << ", "
                << values.v_east << ", "
                << values.v_down << ", "
                << values.heading << ", "
                << values.pitch << ", "
                << values.roll << endl;
            break;
          }
          default:
            return;
        }
      }
    } handler_N;
    
#if 0
    /**
     * SylphideProtocol�`���œ����Ă���C�y�[�W�p�̏�����
     * 
     */
    class SylphideStreamHandler 
        : public Sylphide_Packet_Observer<float_sylph_t> {
      protected:
        typedef Sylphide_Packet_Observer<float_sylph_t> super_t;
        typedef SylphideStreamHandler self_t;
      public:
        bool previous_seek;
        unsigned int invoked;
        SylphideStreamHandler() 
            : super_t(PAGE_SIZE),
            previous_seek(false), invoked(0) {}
        ~SylphideStreamHandler() {}
        
        /**
         * �p�P�b�g�����������ۂɌĂяo�����֐�
         * 
         */
        void operator()(const self_t &observer){
          if(!observer.validate()){return;}
          
        }
    } handler_C;
#endif
    
  public:
    StreamProcessor()
        : super_t(), invoked(0) {
      
    }
    ~StreamProcessor(){}
    
    /**
     * �t�@�C�����̃X�g���[������1�y�[�W�P�ʂŏ������s���֐�
     * 
     * @param in �X�g���[��
     * @return (bool) �����������������ǂ���
     */
    void process(istream &in){
      char buffer[PAGE_SIZE];
      
      while(true){
        int read_count;
        in.read(buffer, PAGE_SIZE);
        read_count = in.gcount();
        if(in.fail() || (read_count == 0)){return;}
        invoked++;
      
        if(options.debug_level){
          cerr << "--read-- : " << invoked << " page" << endl;
          cerr << hex;
          for(int i(0); i < read_count; i++){
            cerr << setfill('0') 
                << setw(2)
                << (unsigned int)((unsigned char)buffer[i]) << ' ';
          }
          cerr << dec;
          cerr << endl;
          
          if(read_count < PAGE_SIZE){
            cerr << "--skipped-- : " << invoked << " page ; count = " << read_count << endl;
          }
        }
      
        switch(buffer[0]){
#define assign_case(type, mark) \
case mark: if(options.page_ ## type){ \
  super_t::process_packet( \
      buffer, read_count, \
      observer_ ## type , previous_seek_next_ ## type, handler_ ## type); \
} \
break;
          assign_case(A, 'A');
          assign_case(G, 'G');
          assign_case(F, 'F');
          assign_case(P, 'P');
          assign_case(M, 'M');
          assign_case(N, 'N');
#undef assign_case
#if 0
          case 'C': if(options.out_C){
            super_t::process_packet(
                buffer, read_count,
                handler_C, handler_C.previous_seek, handler_C);
          }
          break;
#endif
          default: if(options.page_other){
            if(buffer[0] == 'T'){
              stringstream ss;
              ss << hex;
              for(int i(0); i < read_count; i++){
                ss << setfill('0') 
                    << setw(2)
                    << (unsigned int)((unsigned char)buffer[i]) << ' ';
              }
              ss << endl;
              options.out() << ss.str();
            }
          }
        }
      }      
    }
};

int main(int argc, char *argv[]){

  if(argc < 2){
    cerr << "Usage: " << argv[0] << " log.dat [options] [--page=(A|F|M|P)]" << endl;
    return -1;
  }
  
  // �݊����ێ��̂���
  if(strstr(argv[0], "log_AD_CSV")){
    options.page_A = true;
  }else if(strstr(argv[0], "log_F_CSV")){
    options.page_F = true;
  }else if(strstr(argv[0], "log_M_CSV")){
    options.page_M = true;
  }else if(strstr(argv[0], "log_P_CSV")){
    options.page_P = true;
  }
  
  StreamProcessor processor;
  
  int arg_index(2);
  
  // �ʂ̐ݒ�(�݊����m��)
  if(options.page_P){
    // [option = 0:6bytes/packet; 1:8bytes/packet]
    options.page_P_mode = atoi(argv[arg_index++]);
  }
  
  // �I�v�V�����ɂ��o�͂̐ݒ�
  for(; arg_index < argc; arg_index++){
    if(options.check_spec(argv[arg_index])){continue;}
    
    cerr << "Unknown option!! : " << argv[arg_index] << endl;
    return -1;
  }
  
  options.out().precision(10);
  if(options.in_sylphide){
    SylphideIStream sylph_in(options.spec2istream(argv[1]), PAGE_SIZE);
    processor.process(sylph_in);
  }else{
    processor.process(options.spec2istream(argv[1]));
  }
  
  return 0;
}
