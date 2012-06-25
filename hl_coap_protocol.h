/* 
 * File:   hl_coap_protocol.h
 * Author: maxpagel
 *
 * Created on 1. Juni 2012, 10:39
 */

#ifndef _HL_COAP_PROTOCOL_H
#define	_HL_COAP_PROTOCOL_H
#include "external_interface/default_return_values.h"
#include "util/pstl/map_static_vector.h"
#include "util/tuple_store/string_or_int.h"
#include <util/pstl/BitString.h>
#include "util/pstl/pair.h"
#include "transaction_tuple.h"
#include <util/tuple_store/simple_tuple.h>
#include "coap_service_static.h"
#include <util/protobuf/buffer_dynamic.h>

namespace wiselib{
    template<typename OsModel_P,
            typename Broker_P,
            typename Serializer_P,
            typename Allocator_P,
            typename Radio_P = typename OsModel_P::Radio,
            typename Timer_P = typename OsModel_P::Timer,
            typename Rand_P = typename OsModel_P::Rand,
            typename Debug_P = typename OsModel_P::Debug
            >
    class HlCoapProtocol{
        typedef OsModel_P OsModel;
        typedef Radio_P Radio;
        typedef Broker_P broker_t;
        typedef Timer_P timer_t;
        typedef Rand_P rand_t;
        typedef Allocator_P allocator_t;
        typedef Serializer_P serializer_t;
        typedef typename broker_t::string_t string_t;
        typedef typename broker_t::bitstring_t bitstring_t;
        typedef pair<typename Radio::node_id_t,typename broker_t::document_name_t> subscription_t;
        typedef wiselib::MapStaticVector<OsModel,typename broker_t::subscription_id_t,subscription_t ,20 >  subscriptions_map_t;
        typedef CoapServiceStatic<OsModel> coap_service_t;
        typedef wiselib::protobuf::buffer_dynamic<OsModel, allocator_t> buffer_dynamic_t;  
//        typedef TransactionTuple<OsModel,string_t,allocator_t,broker_t> transaction_t;
//        typedef TransactionTuple<OsModel,bitstring_t,allocator_t,broker_t> compressed_transaction_t;
//        typedef wiselib::MapStaticVector<OsModel,int,typename transaction_t::self_pointer_t ,20 >  transaktions_map_t;
//        typedef wiselib::MapStaticVector<OsModel,int,typename compressed_transaction_t::self_pointer_t ,20 >  compressed_transaktions_map_t;

        typedef HlCoapProtocol<OsModel,broker_t,serializer_t,allocator_t,Radio,timer_t,rand_t,Debug_P> self_type;
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
        
        void init(broker_t& broker, typename Radio::self_pointer_t radio, typename allocator_t::self_pointer_t allocator, typename timer_t::self_pointer_t timer,
            typename rand_t::self_pointer_t rand, typename Debug_P::self_pointer_t debug){
            broker_ = &broker;
            radio_ = radio;
            allocator_ = allocator;
            timer_ = timer;
            rand_ = rand;
            debug_ = debug;

            coap_.init(*radio_,*timer_,*rand_);
            coap_.enable_radio();
            coap_.template reg_resource_callback<self_type, &self_type::resource_callback>("http://some/resource/Full",this);
        }

        void get(){
            coap_.template get<self_type, &self_type::received_callback>(0xa47,"http://some/resource/Full","",this,true);
        }



//        template <class T, void (T::*TMethod)( typename coap_service_t::ReceivedMessage& ) >
//	int reg_resource_callback( string_t resource_path, T *callback )
        
    private:
        void resource_callback(typename coap_service_t::ReceivedMessage& message){
            buffer_dynamic_t buffer( allocator_ );
            string_t name(message.message().uri_path().c_str(),message.message().uri_path().length(),allocator_);
            broker_->begin_document(name);
            *buffer = 71;
            ++buffer;
            *buffer = 72;
            ++buffer;
            coap_.reply(message,buffer.data(),buffer.size(),COAP_CODE_VALID);
        }

        void received_callback(typename coap_service_t::ReceivedMessage& message){
            debug_->debug("received reply");
        }


        typename broker_t::self_pointer_t broker_;
        typename Radio::self_pointer_t radio_;
        typename timer_t::self_pointer_t timer_;
        typename rand_t::self_pointer_t rand_;
        typename allocator_t::self_pointer_t allocator_;
        typename Debug_P::self_pointer_t debug_;
        subscriptions_map_t subscriptions_;
        coap_service_t coap_;        
        

//        transaktions_map_t transactions_;
//        compressed_transaktions_map_t compressed_transactions_;


    };
}



#endif	/* _HL_COAP_PROTOCOL_H */

