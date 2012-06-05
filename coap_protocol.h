/*
 * File:   coap_protocol.h
 * Author: anilshanbhag
 *
 * Created on 31. May 2012, 13:53
 */

#ifndef COAP_PROTOCOL_H_
#define COAP_PROTOCOL_H_

#include "external_interface/default_return_values.h"
#include "util/pstl/map_static_vector.h"
#include "request_message.h"
#include "util/tuple_store/string_or_int.h"
#include "util/pstl/BitString.h"
#include "util/pstl/static_string.h"
#include "util/protobuf/buffer_dynamic.h"
#include "util/pstl/pair.h"
#include "insert_message.h"
#include "payload_request_message.h"
#include "transaction_tuple.h"
#include "util/tuple_store/simple_tuple.h"
#include "algorithms/coap/coap.h"

namespace wiselib{

	template<typename OsModel_P,
			typename Broker_P,
			typename Serializer_P,
			typename Allocator_P,
			typename Radio_P = typename OsModel_P::Radio,
			typename Timer_P = typename OsModel_P::Timer,
			typename Debug_P = typename OsModel_P::Debug,
			typename Rand_P = typename OsModel_P::Rand
			>
	class CoapProtocol {

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
        typedef wiselib::MapStaticVector<OsModel,int,typename transaction_t::self_pointer_t ,20 >  transactions_map_t;
        typedef wiselib::MapStaticVector<OsModel,int,typename compressed_transaction_t::self_pointer_t ,20 >  compressed_transactions_map_t;
        typedef RequestMessage<OsModel> request_t;
        typedef InsertMessage<OsModel> insert_message_t;
        typedef PayloadRequestMessage<OsModel> payload_message_t;
        typedef wiselib::protobuf::buffer_dynamic<OsModel, allocator_t> buffer_dynamic_t;

        typedef CoapProtocol<OsModel,broker_t,serializer_t,allocator_t,Radio> self_type;
        typedef self_type* self_pointer_t;

        typedef wiselib::SimpleTuple<Os, COLS, string_t, allocator_t> tuple_t;
        typedef wiselib::Coap<Os, Radio, Os::Timer, Os::Debug, Os::Rand> coap_t;

	public:

        void receive_radio_message(typename Radio::node_id_t source, typename Radio::size_t len, typename Radio::block_data_t *buf)
        {
        	debug_->debug("Got something");
        	if (buf[0] == WISELIB_MID_COAP)
        	{
#ifdef ISENSE
                debug_->debug( "Node %x received msg from %x msg type: CoAP length: %d\n", radio_->id(), source, len );
#else
                debug_->debug("Node received msg from %x | msg type: CoAP | length: %d\n", source, len);
#endif
                debug_hex( buf, len );
                coap_.receiver( &len, buf, &source );
        	}
        }

        void received(typename Radio::node_id_t source, typename Radio::size_t len, typename Radio::block_data_t *buf)
        {
        	debug_->debug("Got something1");
        }

        void init(broker_t& broker, typename allocator_t::self_pointer_t allocator, typename Radio::self_pointer_t radio,
        		typename Timer_P::self_pointer_t timer, typename Debug_P::self_pointer_t debug, typename Rand_P::self_pointer_t rand)
        {

        	// initializing private variables
        	broker_ = &broker;
            allocator_ = allocator;
        	radio_ = radio;
        	radio_->enable_radio();
        	radio_->template reg_recv_callback<self_type, &self_type::received > (this);
        	timer_ = timer;
            debug_ = debug;
            rand_ = rand;
            res = 0;

            // coap necessities
#ifdef ISENSE
            rand_->srand( radio_->id() );
            mid_ = ( uint16_t )rand_->operator()( 65536 / 2 );
#else
            mid_ = ( uint16_t )rand_->operator()( 65536 / 2 );
#endif
        }

		uint8_t add_resource( StaticString name, bool fast_resource, uint16_t notify_time, uint8_t resource_len, uint8_t content_type )
		{
			if (res < CONF_MAX_RESOURCES)
			{
				resources[res].init();
				resources[res].reg_resource(name,fast_resource,notify_time,resource_len,content_type);

				res++;
				return res-1;
			}
			// TODO : See what to do if > max initialized
			return 1;
		}

		template<class T, char* ( T::*TMethod ) ( uint8_t )>
		void add_method( uint8_t resource_id, uint8_t qid, uint8_t method, T *objpnt, StaticString query="")
		{
			// TODO : Check if resource initialized ? Any other loops ?
			resources[resource_id].set_method( qid, method );
			resources[resource_id].template reg_callback<T, TMethod>( objpnt, qid );
			if (method == PUT) resources[resource_id].reg_query( qid, query );
		}

		void coap_start( )
		{
            coap_.init( *radio_, *timer_, *debug_, mid_, resources );
            // radio_->template reg_recv_callback<self_type, &self_type::receive_radio_message > (this);
		}

	private:

        void debug_hex( const uint8_t * payload, typename Radio::size_t length )
        {
           char buffer[2048];
           int bytes_written = 0;
           bytes_written += sprintf( buffer + bytes_written, "DATA:!" );
           for ( size_t i = 0; i < length; i++ )
           {
              bytes_written += sprintf( buffer + bytes_written, "%x!", payload[i] );
           }
           buffer[bytes_written] = '\0';
           debug_->debug( "%s\n", buffer );
        }

        typename broker_t::self_pointer_t broker_;
        typename allocator_t::self_pointer_t allocator_;

        typename Radio::self_pointer_t radio_;
        typename Timer_P::self_pointer_t timer_;
        typename Debug_P::self_pointer_t debug_;
        typename Rand_P::self_pointer_t rand_;

        subscriptions_map_t subscriptions_;
        transactions_map_t transactions_;
        compressed_transactions_map_t compressed_transactions_;

        uint16_t mid_;
        coap_t coap_;
        coap_packet_t packet;
        resource_t resources[CONF_MAX_RESOURCES];
        uint8_t res;

	};
}

#endif
