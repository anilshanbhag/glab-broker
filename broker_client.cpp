
#include <external_interface/external_interface.h>
typedef wiselib::OSMODEL Os;

#include <external_interface/pc/pc_com_uart.h>
#include <external_interface/pc/com_isense_radio.h>
typedef wiselib::PCComUartModel<Os, true> uart_t;

typedef wiselib::ComISenseRadioModel<Os, uart_t> radio_t;

#include <util/allocators/malloc_free_allocator.h>
typedef wiselib::MallocFreeAllocator<Os> Allocator;

#include <util/protobuf/message.h>
#include <util/protobuf/buffer_dynamic.h>

typedef unsigned int int_t;
typedef wiselib::protobuf::Message<Os, Os::block_data_t*, int_t> static_message_t;
typedef wiselib::protobuf::buffer_dynamic<Os, Allocator> buffer_dynamic_t;
typedef wiselib::protobuf::Message<Os, buffer_dynamic_t, int_t> dynamic_message_t;

#include <util/pstl/string_dynamic.h>
typedef wiselib::string_dynamic<Os, Allocator> string_t;

#include "fragmenting_radio.h"
typedef wiselib::FragementingRadio<Os, Allocator, radio_t, Os::Debug> fragmenting_t;

#include <util/tuple_store/simple_tuple.h>
typedef wiselib::SimpleTuple<Os, 5, string_t, Allocator> tuple_t;

#include "swap_request_message.h"
typedef wiselib::SwapRequestMessage<Os, fragmenting_t> swap_message_t;

#define COLS 5
#include "n3reader.h"
//typedef N3Reader<Os,Allocator,string_t,COLS> reader_t;

class App {
	public:
		void init(Os::AppMainParameter& amp) {
			uart_ = &wiselib::FacetProvider<Os, uart_t>::get_facet(amp);
			debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet(amp);
			radio_ = &wiselib::FacetProvider<Os, radio_t>::get_facet(amp);
         timer_ = &wiselib::FacetProvider<Os, Os::Timer>::get_facet( amp);

//         uart_->enable_serial_comm();
			radio_->init(*uart_);
//			radio_->enable_radio();
			
			fragmenting_.init(radio_, debug_, &allocator_);
			fragmenting_.enable_radio();
			
			fragmenting_.reg_recv_callback<App, &App::receive_packet>(this);
			
			request_id = 0;
			size = 0;
			
//			make_request(0x01); // get sensornode metadata
         timer_->set_timer<App,
                         &App::request>( 3000, this, 0 );
//         post_metadata();
		}
      void request(void*){
          post_metadata();
//          Os::Radio::block_data_t message[] = "Let's compare our IDs\0";
//         radio_->send( Os::Radio::BROADCAST_ADDRESS, sizeof(message), message );
      }
		
		void make_request(fragmenting_t::block_data_t request_type) {
			buffer[0] = 70; // Request
			buffer[1] = request_id;
			buffer[2] = request_type;
			size = 3;
		}
      
		void post_metadata( )
    {
        tuple_t t;
        t.set_allocator( &allocator_ );

        //			t[0] = string_t("<foo>", &allocator_);
        //			t[1] = string_t("<bar>", &allocator_);
        //			t[2] = string_t("<baz>", &allocator_);
        //
        //			char x[1] = { 1 | 4 };
        //			t[3] = string_t(x, &allocator_);
        //
        //			t[4] = t[0];

        // protobuf part

        buffer_dynamic_t buf_msg(&allocator_);

						// TODO: its ugly doing this by hand,
						// somehow find a way to use dynamic buffer together
						// with SwapRequestMessage, ideally without too much
						// unecessary copying
						// (see also broker.h)
        *buf_msg = 70;
        ++buf_msg;
        *buf_msg = 0;
        ++buf_msg;
        *buf_msg = 0x06;
        ++buf_msg;

        // reserve 2 bytes for payload length info
        size_t lenpos = buf_msg.position();
        *buf_msg = 0;
        ++buf_msg;
        *buf_msg = 0;
        ++buf_msg;
        int i = 1;

        N3Reader<Os, Allocator, string_t, COLS> reader( "test_data_plain.txt", &allocator_ );
//        for (; reader.ok( ); ++reader, i++ )
//        {

            buffer_dynamic_t statement( &allocator_ );
            dynamic_message_t::write( statement, 1, (*reader)[0] );
            dynamic_message_t::write( statement, 2, (*reader)[1] );
            dynamic_message_t::write( statement, 3, (*reader)[2] );
            dynamic_message_t::write( statement, 4, (*reader)[3] );
            dynamic_message_t::write( statement, 5, (*reader)[4] );

            dynamic_message_t::write( buf_msg, 1, statement.vector( ) );
            printf( "%d %s %s %s %s %s\n",i, (*reader)[0].c_str( ), (*reader)[1].c_str( ), (*reader)[2].c_str( ), (*reader)[3].c_str( ), (*reader)[4].c_str( ) );
//        }


        size_t len = buf_msg.position() - (lenpos + 2);
        
		((swap_message_t*)buf_msg.data())->set_request_opts_length(len);

        swap_message_t *request = (swap_message_t*) buf_msg.data();
        radio_t::block_data_t *p = request->request_opts(),
                                                 *p_end = p + request->request_opts_length();
        printf("cmd type %d request_id %d type %d opt_length %d buffer_len %d %d\n",request->command_type(), request->request_id(),request->request_type(), request->request_opts_length(), len, p_end - p);
        
        insert_protobuf_statements(p, p_end);

        fragmenting_.send(radio_t::BROADCAST_ADDRESS, buf_msg.size(), (radio_t::block_data_t*) buf_msg.data());

        
        

        
        
    }


 //Remove this, just testcode
      void insert_protobuf_statements(radio_t::block_data_t *p, radio_t::block_data_t *p_end) {
			int_t field;
			do {
				buffer_dynamic_t statement(&allocator_);
				switch(static_message_t::field_number(p)) {
					case 1:
						static_message_t::read(p, field, statement.vector());
						insert_protobuf_statement(statement);
						break;
				}
			} while(p < p_end);
		}

		void insert_protobuf_statement(buffer_dynamic_t& buf) {
			int_t field;
			tuple_t t;
			t.set_allocator(&allocator_);
			do {
				string_t s(&allocator_);
				switch(dynamic_message_t::field_number(buf)) {
					case 1: // subject
						dynamic_message_t::read(buf, field, s);
						t[0] = s;
						break;
					case 2: // predicate
						dynamic_message_t::read(buf, field, s);
						t[1] = s;
						break;
					case 3: // object
						dynamic_message_t::read(buf, field, s);
						t[2] = s;
						break;
					case 4: // bitmask
						dynamic_message_t::read(buf, field, s);
						t[3] = s;
						break;
					case 5: //context
						dynamic_message_t::read(buf, field, s);
						t[4] = s;
						break;
				}
			} while(buf.readonly());
         printf( "output %s %s %s %s %s\n", t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ), t[4].c_str( ) );
			
		}

      //end Remove this, just testcode

		
		void receive_packet(fragmenting_t::node_id_t source, fragmenting_t::size_t len, fragmenting_t::block_data_t *buf) {
			if(buf[0] != 71) {
				debug_->debug("WARNING: received message is not a RESPONSE (71) but of type %d", buf[0]);
			}
			if(len < 4) {
				debug_->debug("WARNING: received message is too short");
			}
			
			fragmenting_t::block_data_t req_id, resp_code, payload_len;
			
			req_id = buf[1];
			resp_code = buf[2];
			payload_len = buf[3];
			
			interpret_protobuf(buf + 3, len - 3);
			
		}
		
		void interpret_protobuf(fragmenting_t::block_data_t *buf, size_t len) {
			int_t field;
			string_t s(&allocator_);
			buffer_dynamic_t dynbuf(&allocator_);
			
			fragmenting_t::block_data_t *p = buf;
			
			while(p < buf+len) {
				switch(static_message_t::field_number(p)) {
					case 1:
						static_message_t::read(p, field, s);
						printf("se-id: %s\n", s.c_str());
						break;
					case 2:
						static_message_t::read(p,  field, dynbuf.vector());
						printf("created\n");
						interpret_se_created(dynbuf);
						break;
					case 4:
						static_message_t::read(p,  field, dynbuf.vector());
						printf("description\n");
						interpret_se_created(dynbuf);
						break;
				}
			}
		}
		
		void interpret_se_created(buffer_dynamic_t& dynbuf) {
			int_t field;
			string_t s(&allocator_);
			do {
				switch(dynamic_message_t::field_number(dynbuf)) {
					case 1:
						dynamic_message_t::read(dynbuf, field, s);
						printf("  element: %s\n", s.c_str());
						break;
				}
			} while(dynbuf.readonly());
		}
		
		void interpret_se_description(buffer_dynamic_t& dynbuf) {
			int_t field;
			buffer_dynamic_t dynbuf2(&allocator_);
			do {
				switch(dynamic_message_t::field_number(dynbuf)) {
					case 1:
						printf("  statement\n");
						dynamic_message_t::read(dynbuf, field, dynbuf2.vector());
						interpret_statement(dynbuf2);
						break;
				}
			} while(dynbuf.readonly());
		}
		
		void interpret_statement(buffer_dynamic_t& dynbuf) {
			int_t field;
			string_t s(&allocator_);
			do {
				switch(dynamic_message_t::field_number(dynbuf)) {
					case 1:
						dynamic_message_t::read(dynbuf, field, s);
						printf("    s: %s\n", s.c_str());
						break;
					case 2:
						dynamic_message_t::read(dynbuf, field, s);
						printf("    p: %s\n", s.c_str());
						break;
					case 3:
						dynamic_message_t::read(dynbuf, field, s);
						printf("    o: %s\n", s.c_str());
						break;
				}
			} while(dynbuf.readonly());
		}
			
		
	private:
		fragmenting_t::block_data_t buffer[1024];
		
		uart_t::self_pointer_t uart_;
		Os::Debug::self_pointer_t debug_;
		radio_t::self_pointer_t radio_;
      Os::Timer::self_pointer_t timer_;
		fragmenting_t::block_data_t request_id;
		Allocator allocator_;
		fragmenting_t fragmenting_;
		int size;
};


wiselib::WiselibApplication<Os, App> app;
void application_main(Os::AppMainParameter& amp) {
        app.init(amp);
}


