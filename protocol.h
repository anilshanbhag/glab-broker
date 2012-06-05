/* 
 * File:   protocol.h
 * Author: maxpagel
 *
 * Created on 9. Mai 2012, 11:18
 */

#ifndef _PROTOCOLL_H
#define	_PROTOCOLL_H

#include "external_interface/default_return_values.h"
#include "util/pstl/map_static_vector.h"
#include "request_message.h"
#include "util/tuple_store/string_or_int.h"
#include <util/pstl/BitString.h>
#include <util/protobuf/buffer_dynamic.h>
#include "util/pstl/pair.h"
#include "insert_message.h"
#include "payload_request_message.h"
#include "transaction_tuple.h"
#include "util/tuple_store/simple_tuple.h"

namespace wiselib{
    
    template<typename OsModel_P,
            typename Broker_P,
            typename Serializer_P,
            typename Allocator_P,
            typename Radio_P = typename OsModel_P::Radio,
            typename Debug_P = typename OsModel_P::Debug
            >
    class Protocol{

        typedef OsModel_P OsModel;
        typedef Radio_P Radio;
        typedef Broker_P broker_t;
        typedef Allocator_P allocator_t;
        typedef Serializer_P serializer_t;
        typedef typename broker_t::string_t string_t;
        typedef typename broker_t::bitstring_t bitstring_t;
        typedef pair<typename Radio::node_id_t,typename broker_t::document_name_t> subscription_t;
        typedef wiselib::MapStaticVector<OsModel,typename broker_t::subscription_id_t,subscription_t ,20 >  subscriptions_map_t;
        typedef TransactionTuple<OsModel,string_t,allocator_t,broker_t> transaction_t;
        typedef TransactionTuple<OsModel,bitstring_t,allocator_t,broker_t> compressed_transaction_t;
        typedef wiselib::MapStaticVector<OsModel,int,typename transaction_t::self_pointer_t ,20 >  transaktions_map_t;
        typedef wiselib::MapStaticVector<OsModel,int,typename compressed_transaction_t::self_pointer_t ,20 >  compressed_transaktions_map_t;
        typedef RequestMessage<OsModel> request_t;
        typedef InsertMessage<OsModel> insert_message_t;
        typedef PayloadRequestMessage<OsModel> payload_message_t;
        typedef wiselib::protobuf::buffer_dynamic<OsModel, allocator_t> buffer_dynamic_t;       
        typedef Protocol<OsModel,broker_t,serializer_t,allocator_t,Radio> self_type;
        typedef self_type* self_pointer_t;
        typedef wiselib::SimpleTuple<Os, COLS, string_t, allocator_t> tuple_t;

    public:
        enum{
            GET_DOCUMENT = 11,
            SUBSCRIBE = 12,
            UNSUBSCRIBE = 13,
            INSERT = 14,
            ERASE_DOCUMENT = 15,
            STRING = 10,
            COMMAND = 9,
            SUBJECT = 1,
            PREDICATE = 2,
            OBJECT = 4,
            DOCUMENT_NAME = 8,
            INSERT_COMMAND = 16,
            ERASE_COMMAND = 32,
            LOG_COMMAND = 64
        };

        void receive_radio_message(typename Radio::node_id_t source, typename Radio::size_t length,
                typename Radio::block_data_t *buffer)
        {
           request_t *request = (request_t*) buffer;
           if(request->command_type() == SSD_REST_REQUEST)
           {             

               switch ( request->request_type())
               {
                   case GET_DOCUMENT:
                   {
                       payload_message_t *msg = (payload_message_t*) buffer;
                       string_t name((char*)msg->payload(),msg->payload_length(),allocator_);
/*                        if(msg->flags() & payload_message_t::FLAG_COMPRESSED) { */
/*                           send_data(source,broker_->begin_compressed_document(name),broker_->end_compressed_document(name),request->request_id()); */
/*                        } */
/*                        else { */
                          send_data_tx(source,broker_->begin_document(name),broker_->end_document(name),request->request_id());
/*                        } */
                   }
                   break;
                   case SUBSCRIBE:
                   {
						//debug_->debug("subs");
                       payload_message_t *msg = (payload_message_t*) buffer;
                       string_t name((char*)msg->payload(),msg->payload_length(),allocator_);
                  //     typename broker_t::subscription_id_t subscription_id = broker_->template subscribe<self_type,&self_type::push_document>(this);
                       //subscriptions_[subscription_id] = subscription_t(source,name);
                   }
                   break;
                   case UNSUBSCRIBE:
                   {
                       payload_message_t *msg = (payload_message_t*) buffer;
                       typename broker_t::subscription_id_t id = read<OsModel, typename Radio::block_data_t,
                               typename broker_t::subscription_id_t>( msg->payload());
                       broker_->unsubscribe(id);
                       subscriptions_.erase(id);
                   }
                   break;
                   case INSERT:
                   {                        
                       insert_message_t *insert = (insert_message_t*) buffer;
                       
                       if(insert->flags() & insert_message_t::FLAG_COMPRESSED) {
						//debug_->debug("ins compr");
                          typename compressed_transaktions_map_t::iterator transaction = compressed_transactions_.find(insert->transaction_id());
                          if(transaction == compressed_transactions_.end()){
                              compressed_transactions_[insert->transaction_id()] = allocator_->template allocate<compressed_transaction_t>();
                              transaction = compressed_transactions_.find(insert->transaction_id());
                              (*transaction).second->init(broker_,allocator_);
                          }
                          if(insert->insert_type() == STRING){
                              //string_t string((char*)insert->payload(),((insert->payload_length() + 7)  / 8) ,allocator_);
                              bitstring_t string(allocator_);
                              for(int i=0; i<((insert->payload_length() + 7)  / 8); i++) {
                                 string.push_byte((insert->payload())[i]);
                              }
                              string.resize(insert->payload_length());
                              switch(insert->type_action()){
                                  case SUBJECT:
                                      (*transaction).second->set_subject(string);
                                  break;
                                  case OBJECT:
                                      (*transaction).second->set_object(string);
                                  break;
                                  case PREDICATE:
                                      (*transaction).second->set_predicate(string);
                                  break;
                                  case DOCUMENT_NAME: {
                                    string_t s((char*)insert->payload(),insert->payload_length()  ,allocator_);
                                      (*transaction).second->set_document_name(s);
                                  }
                                  break;
                              }

                          }else if(insert->insert_type() == COMMAND){
                              (*transaction).second->set_command(insert->type_action());
                          }
                          if((*transaction).second->execute_compressed(debug_)){
                              allocator_->free((*transaction).second);
                              transactions_.erase((*transaction).first);                           
                          }
                       }
                       else { // uncompressed
                       
						//debug_->debug("ins clean");
                          typename transaktions_map_t::iterator transaction = transactions_.find(insert->transaction_id());
                          if(transaction == transactions_.end()){
                              transactions_[insert->transaction_id()] = allocator_->template allocate<transaction_t>();
                              transaction = transactions_.find(insert->transaction_id());
                              (*transaction).second->init(broker_,allocator_);
                          }
                          if(insert->insert_type() == STRING){
                              string_t string((char*)insert->payload(),insert->payload_length()  ,allocator_);
                              switch(insert->type_action()){
                                  case SUBJECT:
                                      (*transaction).second->set_subject(string);
                                  break;
                                  case OBJECT:
                                      (*transaction).second->set_object(string);
                                  break;
                                  case PREDICATE:
                                      (*transaction).second->set_predicate(string);
                                  break;
                                  case DOCUMENT_NAME:
                                      (*transaction).second->set_document_name(string);
                                  break;
                              }

                          }else if(insert->insert_type() == COMMAND){
                              (*transaction).second->set_command(insert->type_action());
                          }
                          if((*transaction).second->execute( debug_)){
                              allocator_->free((*transaction).second);
                              transactions_.erase((*transaction).first);                           
                          }
                       } // if/else

                   }
                   break;
                   case ERASE_DOCUMENT:
                   {
                       payload_message_t *msg = (payload_message_t*) buffer;
                       string_t name((char*)msg->payload(),msg->payload_length(),allocator_);
                       broker_->erase_document(name);
                   }
                   break;
                   default:
						//debug_->debug("crap");
//                       TODO irgendwas sinvolles tun
                   break;
               }
               

           }
        }
/*
        void push_document(typename broker_t::document_name_t name){
            typename subscriptions_map_t::iterator it_end = subscriptions_.end();
            for(typename subscriptions_map_t::iterator it=subscriptions_.begin();it!=it_end;++it){
                if((*it).second.second==name)
                    send_data((*it).second.first,broker_->begin_document(name),broker_->end_document(name),0);
            }
        }
*/
        void init(broker_t& broker, typename Radio::self_pointer_t radio, typename allocator_t::self_pointer_t allocator, typename Debug_P::self_pointer_t debug){
            broker_ = &broker;
            radio_ = radio;
            allocator_ = allocator;
            debug_ = debug;
            radio_->template reg_recv_callback<self_type, &self_type::receive_radio_message > (this);
        }

        

    private:

        void send_data_tx(typename Radio::node_id_t destination, typename broker_t::iterator it, typename broker_t::iterator it_end,uint8_t request_id )
        {
			//debug_->debug("send tx");
            int i = 0;
            for (/*it_*/; it != it_end; ++it){
				//debug_->debug("send %s %s %s",(*it)[0].c_str( ),(*it)[1].c_str( ),(*it)[2].c_str( ));
				
                tuple_t t;
                t.set_allocator( allocator_ );
                insert_message_t msg;
                msg.set_command_type(SSD_REST_REQUEST);
                msg.set_request_id(request_id);
                msg.set_request_type(INSERT);
                msg.set_transaction_id(i%20);
                msg.set_Insert_type(COMMAND);
                msg.set_type_action(LOG_COMMAND);
                
                msg.set_flags(0); //insert_message_t::FLAG_COMPRESSED
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                msg.set_Insert_type(STRING);
                msg.set_type_action(SUBJECT);
                msg.set_payload((*it)[0].length(),(uint8_t*)(*it)[0].c_str( ));
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                
                msg.set_Insert_type(STRING);
                msg.set_type_action(PREDICATE);
                msg.set_payload((*it)[1].length(),(uint8_t*)(*it)[1].c_str( ));
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                
                msg.set_Insert_type(STRING);
                msg.set_type_action(OBJECT);
                msg.set_payload((*it)[2].length(),(uint8_t*)(*it)[2].c_str( ));
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                typename broker_t::string_t doc_name("NODE",allocator_);
                msg.set_Insert_type(STRING);
                msg.set_type_action(DOCUMENT_NAME);
                msg.set_payload(doc_name.length(),(uint8_t*)doc_name.c_str());
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif
                    
                ++i;
            }
			//debug_->debug("send tx done");
        
        }
        
        void send_data_tx(typename Radio::node_id_t destination, typename broker_t::compressed_iterator it, typename broker_t::compressed_iterator it_end,uint8_t request_id )
        {
			//debug_->debug("send tx compressed");
            int i = 0;
            for (/*it_*/; it != it_end; ++it){
			//	debug_->debug("send %s %s %s",(*it)[0].c_str( ),(*it)[1].c_str( ),(*it)[2].c_str( ));
				
                tuple_t t;
                t.set_allocator( allocator_ );
                insert_message_t msg;
                msg.set_command_type(SSD_REST_REQUEST);
                msg.set_request_id(request_id);
                msg.set_request_type(INSERT);
                msg.set_transaction_id(i%20);
                msg.set_Insert_type(COMMAND);
                msg.set_type_action(LOG_COMMAND);
                
                msg.set_flags(insert_message_t::FLAG_COMPRESSED);
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                msg.set_Insert_type(STRING);
                msg.set_type_action(SUBJECT);
                msg.set_payload((*it)[0].size_bytes(),(uint8_t*)(*it)[0].data(), (*it)[0].length());
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                
                msg.set_Insert_type(STRING);
                msg.set_type_action(PREDICATE);
                msg.set_payload((*it)[1].size_bytes(),(uint8_t*)(*it)[1].data(), (*it)[1].length());
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                
                msg.set_Insert_type(STRING);
                msg.set_type_action(OBJECT);
                msg.set_payload((*it)[2].size_bytes(),(uint8_t*)(*it)[2].data(), (*it)[2].length());
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif

                typename broker_t::string_t doc_name("NODE",allocator_);
                msg.set_Insert_type(STRING);
                msg.set_type_action(DOCUMENT_NAME);
                msg.set_payload(doc_name.length(),(uint8_t*)doc_name.c_str());
                #ifdef DRY
                    receive_radio_message(0x8,msg.buffer_size(),(typename Radio::block_data_t*) &msg);
                #else
                    radio_->send( Radio::BROADCAST_ADDRESS, msg.buffer_size(), (typename Radio::block_data_t*)&msg);
                #endif
                    
                ++i;
            }
        
        }
/*
        void send_data(typename Radio::node_id_t destination, typename broker_t::iterator it, typename broker_t::iterator it_end,uint8_t request_id )
        {
	  bool more;
	  serializer_t serializerx(it,it_end,allocator_);
	  do {
	    buffer_dynamic_t buffer( allocator_ );
	    more = serializerx.fill_buffer(buffer,100,debug_);
	  } while( more );
	  //abort();


            // TODO: its ugly doing this by hand,
           // somehow find a way to use dynamic buffer together
           // with SwapRequestMessage
            buffer_dynamic_t buffer(allocator_);
           *buffer = 71;
           ++buffer;
           *buffer = request_id;
           ++buffer;
           *buffer =  (typename Radio::block_data_t) DefaultReturnValues<OsModel>::SUCCESS;
           ++buffer;

           // reserve 2 bytes for payload length info
           size_t lenpos = buffer.position();
           *buffer = 0;
           ++buffer;
           *buffer = 0;
           ++buffer;

	    serializer_t serializer(it,it_end,allocator_);
	    serializer.fill_buffer(buffer,0,debug_);

           
           uint16_t len = buffer.position() - (lenpos + 2);
           ((payload_message_t*)buffer.data())->set_payload_length(len);

           

	    // radio_->send( Radio::BROADCAST_ADDRESS, buffer.size(), (typename Radio::block_data_t*)buffer.data());

//           payload_message_t *msg = (payload_message_t*) buffer.data();
//           if(msg->command_type() == SSD_REST_RESPONSE){
//               serializer_t serializer(allocator_);
//               serializer.deserialize(msg->payload(),msg->payload()+msg->payload_length(),debug_);
//           }

        }
        */

        typename broker_t::self_pointer_t broker_;
        typename Radio::self_pointer_t radio_;
        typename allocator_t::self_pointer_t allocator_;
        typename Debug_P::self_pointer_t debug_;
        subscriptions_map_t subscriptions_;
        transaktions_map_t transactions_;
        compressed_transaktions_map_t compressed_transactions_;
    };
}


#endif	/* _PROTOCOLL_H */

