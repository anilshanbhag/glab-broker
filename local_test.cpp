/*
 * File:   local_test.cpp
 * Author: maxpagel & anilshanbhag
 *
 * Created on 18. April 2012, 13:13
 * Last Modified on 23 May 2012, 14:48
 */

/**
 * For "pc" module: make pc
 * For "isense" module: make isense.5148
 * To simulate on pc :
 * uncomment flag #define DRY & run make pc
 */

/**
 *  TODO : Problem when both TS_SDP and LS_SDP enabled.
 */

#define TS_MALLOC_FREE_ALLOCATOR 1

#include "external_interface/external_interface.h"
#include "external_interface/external_interface_testing.h"

#include "util/pstl/static_string.h"
#include "broker.h"
#include "util/wisebed_node_api/command_types.h"
#include "util/delegates/delegate.hpp"
#include <util/tuple_store/simple_tuple.h>

#include "external_interface/default_return_values.h"

typedef wiselib::OSMODEL Os;

#ifdef PC
    #include <external_interface/pc/pc_com_uart.h>
    #include <external_interface/pc/com_isense_radio.h>
    #ifdef DRY
        typedef Os::Uart uart_t;
    #else
        typedef wiselib::PCComUartModel<Os, true > uart_t;
    #endif
    //typedef wiselib::ComISenseRadioModel<Os, uart_t> radio_t;
#else
    //typedef Os::Radio radio_t;
#endif

// To simulate tuplestore add and get on pc
// #define DRY

#ifndef PC
    // To enable temperature sensor
	// #define TS_SDP

    // To enable light sensor
	#define LS_SDP
#endif

// #define USE_SHDT

#if defined(PC) || defined(ISENSE) || defined(SHAWN)
    typedef Os::Rand rand_t;
#else
    #include <external_interface/fake_rand.h>
    typedef wiselib::FakeRandModel<Os> rand_t;
#endif

typedef Os::Timer Timer;
typedef Os::Debug debug_t;

#include "algorithms/coap/coap.h"
typedef wiselib::Coap<Os, Os::Radio, Os::Timer, Os::Debug, Os::Rand> coap_t;

#if TS_MALLOC_FREE_ALLOCATOR
    #define FIRST_FIT_ALLOCATOR 0
    #include <util/allocators/malloc_free_allocator.h>
    typedef wiselib::MallocFreeAllocator<Os> allocator_t;
#else
    #define FIRST_FIT_ALLOCATOR 1
    #include <util/allocators/first_fit_allocator.h>
    typedef wiselib::FirstFitAllocator<Os, TS_MEMORY_SIZE, TS_MAX_CHUNKS> allocator_t;
#endif

#if FIRST_FIT_ALLOCATOR
    template<>
    allocator_t allocator_t::self_pointer_t::instance = allocator_t( );
#else
    allocator_t allocator_instance;
#endif




#define COLS 4

#include <util/pstl/string_dynamic.h>
typedef wiselib::string_dynamic<Os, allocator_t> string_t;
typedef wiselib::SimpleTuple<Os, COLS, string_t, allocator_t> tuple_t;

#include <util/pstl/BitString.h>
typedef wiselib::BitString_dynamic<Os, allocator_t> bitstring_t;

#include <util/pstl/bitstring_static_view.h>
typedef wiselib::bitstring_static_view<Os, bitstring_t> bitstring_static_t;

#define AVLTREE_CHECK 0
#define AVLTREE_DEBUG 0
#define AVLTREE_GRAPHVIZ 0
#include <util/pstl/avl_tree.h>
typedef wiselib::AVLTree<Os, allocator_t> avl_tree_t;
#include <util/pstl/list_dynamic.h>
typedef wiselib::list_dynamic<Os,Os::block_data_t*,allocator_t> dyn_list_t;
//typedef avl_tree_t tuple_container_t;
typedef dyn_list_t tuple_container_t;

#include <util/tuple_store/dictionary.h>
typedef wiselib::Dictionary<Os, bitstring_t, allocator_t> bitstring_dictionary_t;

#include <util/tuple_store/PrescillaDict_2.h>
typedef wiselib::PrescillaDict2<Os, allocator_t, bitstring_t> prescilla_dictionary_t;

typedef prescilla_dictionary_t dictionary_t;

#include <util/tuple_store/tuple_store.h>
typedef wiselib::TupleStore<Os, COLS, allocator_t, tuple_container_t> tuple_store_t;

#include <util/tuple_store/dictionary_tuple_store.h>
typedef wiselib::DictionaryTupleStore<Os, COLS, allocator_t, tuple_store_t, dictionary_t> dictionary_store_t;

#include <util/tuple_store/Coder.h>
typedef wiselib::Coder<Os, allocator_t> huffmann_codec_t;

#include <util/tuple_store/null_codec.h>
typedef wiselib::NullCodec<Os, allocator_t> null_codec_t;

#include <util/tuple_store/codec_tuple_store.h>
typedef wiselib::CodecTupleStore<Os, dictionary_store_t, huffmann_codec_t, null_codec_t, allocator_t> codec_store_t;

#include <util/tuple_store/simple_tuple.h>

typedef wiselib::Broker<Os, codec_store_t, allocator_t, uint16_t> broker_t;

#include <util/protobuf/buffer_dynamic.h>

typedef unsigned int int_t;
typedef wiselib::protobuf::buffer_dynamic<Os, allocator_t> buffer_dynamic_t;


#ifdef USE_SHDT
    #include "shdt_rdf_serializer.h"
    typedef wiselib::ShdtRdfSerializer<Os, broker_t::iterator, buffer_dynamic_t, allocator_t, broker_t::string_t> serializer_t;
#else
    #include "protobuf_rdf_serializer.h"
    typedef wiselib::ProtobufRdfSerializer<Os, broker_t::iterator, buffer_dynamic_t, allocator_t> serializer_t;
#endif

#include "coap_protocol.h"
typedef wiselib::CoapProtocol<Os, broker_t, serializer_t, allocator_t> protocol_t;

#include "insert_message.h"
typedef wiselib::InsertMessage<Os> insert_message_t;

#include "payload_request_message.h"
typedef wiselib::PayloadRequestMessage<Os> payload_message_t;

//#include "util/pstl/map_static_vector.h"
//typedef wiselib::MapStaticVector<Os,typename broker_t::document_name_t,typename broker_t::bitmask_t ,10 >  documents_map_t;


#ifndef PC
	#include "semantic_data_provider.h"
    #ifdef TS_SDP
        #include "external_interface/isense/isense_temperature_callback_sensor.h"
        typedef wiselib::iSenseTemperatureCallbackSensor<Os> TempSensor_t;
        typedef wiselib::SemanticDataProvider<Os, broker_t, TempSensor_t, allocator_t> temp_data_provider_t;
    #endif
    #ifdef LS_SDP
        #include "external_interface/isense/isense_light_callback_sensor.h"
        typedef wiselib::iSenseLightCallbackSensor<Os> LightSensor_t;
        typedef wiselib::SemanticDataProvider<Os, broker_t, LightSensor_t, allocator_t> light_data_provider_t;
    #endif
#endif

allocator_t::self_pointer_t allocator_ = &allocator_instance;

class IoTTest
{
public:

    void init( Os::AppMainParameter& value )
    {

#if !TS_NOTHING

    #if !FIRST_FIT_ALLOCATOR
        allocator_ = &allocator_instance;
    #endif

        timer_ = &wiselib::FacetProvider<Os, Os::Timer>::get_facet( value );
        debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
        clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet( value );
        radio_ = &wiselib::FacetProvider<Os, Os::Radio>::get_facet( value );
        rand_ = &wiselib::FacetProvider<Os, Os::Rand>::get_facet( value );
	#ifdef PC
        mid_ = ( uint16_t )rand_->operator()( 65536 / 2 );
	#else
        mid_ = ( uint16_t )rand_->operator()( radio_->id() / 2 );
	#endif

    #ifdef PC
        uart_ = &wiselib::FacetProvider<Os, uart_t>::get_facet( value );
        #ifndef DRY
            uart_->set_address( "/dev/ttyUSB0" );
            uart_->enable_serial_comm( );
        #endif
        radio_->init( *uart_ );
    #endif
    #ifndef DRY
        radio_->enable_radio( );
    #endif

    #ifndef PC
		#ifdef TS_SDP
			tempsensor_ = &wiselib::FacetProvider<Os, TempSensor_t>::get_facet( value );
		#endif
		#ifdef LS_SDP
			lightsensor_ = &wiselib::FacetProvider<Os, LightSensor_t>::get_facet( value );
		#endif
    #endif

        init_tuple_store<dictionary_store_t::internal_tuple_t > ();
        broker_.init( *debug_, *timer_, codec_store_, allocator_ );
        protocol_.init( broker_, allocator_, radio_, timer_, debug_, rand_ );
        add_resources();
        protocol_.coap_start();

    #if !defined(PC) && (defined(TS_SDP) || defined(LS_SDP))
        init_dataproviders( );
    #endif

        timer_->set_timer<IoTTest,
               &IoTTest::execute > (2000, this, 0);
#ifndef PC
        timer_->set_timer<IoTTest,
               &IoTTest::get_light2 > (4000, this, 0);
        timer_->set_timer<IoTTest,
                 &IoTTest::debug_printer> (10000, this, 0);
#endif
        debug_->debug("CoAP application bootin! %d\n", mid_);

#endif
    }

#if !TS_CODESIZE

    void execute( void* )
    {
#ifdef PC
        send_coap( );
        // broker_t::string_t doc_name( "NODE", allocator_ );
        // request_document( doc_name );
        timer_->set_timer<IoTTest,
               &IoTTest::execute > (1000, this, 0);
#endif // PC
    }
#endif // TS_CODESIZE

#ifndef PC
    #ifdef TS_SDP
        TempSensor_t::self_pointer_t tempsensor_;
        temp_data_provider_t temp_data_provider_;
    #endif
    #ifdef LS_SDP
        LightSensor_t::self_pointer_t lightsensor_;
        light_data_provider_t light_data_provider_;
    #endif

    void init_dataproviders( )
    {
    #ifdef TS_SDP
        temp_data_provider_.init(
                                  string_t( "ex:Sensor1234", allocator_ ),
                                  string_t( "SENS", allocator_ ),
                                  string_t( "ssn:Temperature", allocator_ ),
                                  &broker_,
                                  tempsensor_,
                                  allocator_, debug_, timer_
                                  );
    #endif
    #ifdef LS_SDP
        light_data_provider_.init(
                                  string_t( "ex:Sensor1235", allocator_ ),
                                  string_t( "SENS", allocator_ ),
                                  string_t( "ssn:Light", allocator_ ),
                                  &broker_,
                                  lightsensor_,
                                  allocator_, debug_, timer_
                                  );
    #endif
        debug_->debug("Data Providers Initialized\n");
    }
#endif

#if !TS_CODESIZE

    template<typename Tuple>
    void init_tuple_store( )
    {
        tuple_container_.init(
                               allocator_,
                               tuple_container_t::comparator_t::from_function<
								   tuple_container_t::ptr_cmp_comparator<Tuple>
                               >()
                              );

        tuple_store_.init(
                           allocator_, &tuple_container_,debug_
                           );
        null_codec_t::self_pointer_t null_codec = allocator_->allocate<null_codec_t > ();

        dictionary_t::self_pointer_t dictionary = allocator_->allocate<dictionary_t > ();
        dictionary->init( allocator_ );

        dictionary_store_t::self_pointer_t dictionary_store = allocator_->allocate<dictionary_store_t > ();
        dictionary_store->init(
                                allocator_, dictionary, &tuple_store_
                                );

        huffmann_codec_t::self_pointer_t huffmann_codec = allocator_->allocate<huffmann_codec_t > ();
        huffmann_codec->set_allocator( allocator_ );

        null_codec->set_allocator( allocator_ );

        codec_store_ = allocator_->allocate<codec_store_t > ();
        codec_store_->init(
                            dictionary_store, huffmann_codec, null_codec, allocator_
                            );

    }

    void add_resources( )
    {
    	uint8_t rid = protocol_.add_resource( "sensors/light", true, 120, 5, TEXT_PLAIN );
    	protocol_.add_method<IoTTest, &IoTTest::get_light>( rid, 0, GET, this);
    }

#ifdef PC
    void send_coap( )
    {
        char path[] = "sensors/light\0";
        char payload[] = "0";
        Os::Radio::block_data_t buf[100];
        uint8_t buf_len;

#ifdef ISENSE
        uint16_t host = 0x9979;
#else
        uint16_t host = 1;
#endif
        uint8_t token[8];
        token[0] = 0xc1;
        token[1] = 0x45;
        token[2] = 0xaf;
        packet.init();
        packet.set_type( CON );
        packet.set_code( GET );
        packet.set_mid( mid_++ );
        // bytes_written += sprintf( buffer + bytes_written, "" );

        packet.set_uri_host( host );
        packet.set_uri_path_len( sizeof( path ) - 1 );
        packet.set_uri_path( path );
        packet.set_observe( 0 );
        packet.set_token_len( 3 );
        packet.set_token( token );

        packet.set_option( URI_HOST );
        packet.set_option( URI_PATH );
        packet.set_option( OBSERVE );
        packet.set_option( TOKEN );

        packet.set_payload( ( uint8_t * )payload );
        packet.set_payload_len( sizeof( payload ) );
        buf_len = packet.packet_to_buffer( buf );
        debug_->debug("Coap message buffer: %s\n", buf);
#ifdef DRY
        protocol_.receive_radio_message( 0x8, buf_len, buf);
#else
        debug_->debug("Sending messageSize: %d \n", buf_len);
        radio_->send( Os::Radio::BROADCAST_ADDRESS, buf_len, buf );
#endif
        //timer_->set_timer<CoapApplication, &CoapApplication::simple_send>( 30000, this, 0 );
    }
#endif //PC

	void debug_printer(void* ) {
	    debug_->debug("About to debug : Brace Yourself\n");
        codec_store_t::iterator it_end = codec_store_->end( );
        for ( codec_store_t::iterator it = codec_store_->begin( ); it != it_end; ++it )
        {
            tuple_t t;
            t.set_allocator( allocator_ );
            t[0] = (*it)[0];
            t[1] = (*it)[1];
            t[2] = (*it)[2];
            t[3] = (*it)[3];

            debug_->debug( "%s %s %s %s \n",t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ) );
        }
        timer_->set_timer<IoTTest,
                &IoTTest::debug_printer> (10000, this, 0);
	}

    char* get_light( uint8_t method )
    {
       tuple_t t;
       t[0] = "ex:Sensor1235";
       t[1] = "<http://www.loa-cnr.it/ontologies/DUL.owl#hasValue>";
       t.set_wildcard(2, true);
       t.set_wildcard(3, true);

       codec_store_t::iterator it = codec_store_->find( t );

       debug_->debug( "current light value = %s\n", (*it)[2].c_str() );
       return (*it)[2].c_str();
    }

    void get_light2( void* )
    {
       tuple_t t;
       t[0] = "ex:Sensor1235";
       t[1] = "<http://www.loa-cnr.it/ontologies/DUL.owl#hasValue>";
       t.set_wildcard(2, true);
       t.set_wildcard(3, true);

       codec_store_t::iterator it = codec_store_->find( t );

       debug_->debug( "current light value = %s\n", (*it)[2].c_str() );
       // return (*it)[2].c_str();
    }


#endif // !TS_NOTHING
private:

    Os::Timer::self_pointer_t timer_;
    Os::Debug::self_pointer_t debug_;
    Os::Clock::self_pointer_t clock_;
    Os::Radio::self_pointer_t radio_;
    Os::Rand::self_pointer_t rand_;

#if !TS_NOTHING
    coap_t coap_;
    coap_packet_t packet;

    broker_t broker_;
    codec_store_t::self_pointer_t codec_store_;
    tuple_container_t tuple_container_;
    tuple_store_t tuple_store_;
    protocol_t protocol_;

    uint16_t mid_;
    char data_[128];

#ifdef PC
    uart_t::self_pointer_t uart_;
#endif
#endif
};


IoTTest routing_app;
// --------------------------------------------------------------------------

void application_main( Os::AppMainParameter& value )
{
    routing_app.init( value );
};
