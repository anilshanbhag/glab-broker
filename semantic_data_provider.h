/*
 * File:   SemanticDataProvider.h
 * Author: maxpagel
 *
 * Created on 19. April 2012, 13:42
 */

#ifndef _SEMANTICDATAPROVIDER_H
#define	_SEMANTICDATAPROVIDER_H
#include "util/delegates/delegate.hpp"
#include "util/pstl/string_dynamic.h"

namespace wiselib
{

    template<typename OsModel_P,
    typename Broker_P,
    typename Sensor_P,
    typename Allocator_P,
    typename Debug_P = typename OsModel_P::Debug,
    typename Timer_P = typename OsModel_P::Timer
    >
    class SemanticDataProvider
    {
        typedef Broker_P Broker;
        typedef Sensor_P Sensor;
        typedef OsModel_P Os;
        typedef Timer_P Timer;
        typedef Allocator_P Allocator;

        typedef typename Broker::bitmask_t bitmask_t;

        typedef typename Broker_P::tuple_store_t TupleStore_t;
        typedef typename Broker_P::tuple_store_t tuple_store_t;
        typename TupleStore_t::self_pointer_t tuple_store_;
        //typename TempSensor_t::self_pointer_t temp_sensor_;
        typedef SemanticDataProvider<Os, Broker, Sensor, Allocator> self_type;

        //typedef iSenseTemperatureCallbackSensor<Os> TempSensor_t; //TODO generalize
        typedef string_dynamic<Os,Allocator> string_t;
        typedef typename TupleStore_t::Tuple tuple_t;

    public:

        enum {
            COL_SUBJECT = Broker::COL_SUBJECT,
            COL_PREDICATE = Broker::COL_PREDICATE,
            COL_OBJECT = Broker::COL_OBJECT,
            COL_BITMASK = Broker::COL_BITMASK
        };

        SemanticDataProvider()
        {
        }


        SemanticDataProvider(typename Allocator::self_pointer_t alloc) : allocator_(alloc)
        {
            //uri_me = string_t("<sensor23>", allocator_);
            uri_hasValue = string_t("<http://www.loa-cnr.it/ontologies/DUL.owl#hasValue>", allocator_);
            uri_date = string_t("<http://purl.org/dc/terms/date>", allocator_);
            uri_type = string_t("rdf:type", allocator_);
            uri_sensor = string_t("ssn:Sensor", allocator_);
            uri_observedProperty = string_t("ssn:observedProperty", allocator_);
            uri_uomInUse = string_t("ssn:uomInUse", allocator_);
            time_ = 0;
            busy_ = false;
        }

        void add_static_info() {
            /*
               intrinsic:

               <http://spitfire-project.eu/sensor/SENSORNAME> ns0:type <http://purl.oclc.org/NET/ssnx/ssn#Sensor> ;
                                               ns1:observedProperty <http://spitfire-project.eu/property/Temperature> ;
                                               ns2:uomInUse <http://spitfire-project.eu/uom/Centigrade> ;
                                               ns3:hasValue "10.2" ;

             */
            bitmask_t ic = mask_intrinsic_ | mask_complete_;
            add_tuple(me_, uri_type, uri_sensor, ic);
            add_tuple(me_, uri_observedProperty, observed_property_, ic);
            add_tuple(me_, uri_uomInUse, uom_, ic);

        }

        void add_tuple(string_t s, string_t p, string_t o, bitmask_t mask) {
            tuple_t t;
            t.set_allocator(allocator_);
            t[COL_SUBJECT] = s;
            t[COL_PREDICATE] = p;
            t[COL_OBJECT] = o;
            Broker::set_bitmask(t, mask);
            tuple_store_->insert(t);
        }


        void init(
                string_t uri,
                string_t docname_base,
                string_t observedProperty,
                typename Broker::self_pointer_t broker,
                typename Sensor::self_pointer_t sensor,
                typename Allocator::self_pointer_t allocator,
                typename Debug_P::self_pointer_t debug,
                typename Timer::self_pointer_t timer
                )
        {
            timer_ = timer;
            debug_ = debug;
            broker_ = broker;
            tuple_store_ = broker->tuple_store();
            sensor_ = sensor;
            allocator_ = allocator;
            sensor_->enable();

            uri_hasValue = string_t("<http://www.loa-cnr.it/ontologies/DUL.owl#hasValue>", allocator_);
            uri_date = string_t("<http://purl.org/dc/terms/date>", allocator_);
            uri_type = string_t("rdf:type", allocator_);
            uri_sensor = string_t("ssn:Sensor", allocator_);
            uri_observedProperty = string_t("ssn:observedProperty", allocator_);
            uri_uomInUse = string_t("ssn:uomInUse", allocator_);
            time_ = 0;
            busy_ = false;

            me_ = uri;
            observed_property_ = observedProperty;

            string_t s = docname_base;
            s.append(string_t(postfix_intrinsic_, allocator_));
            mask_intrinsic_ = broker_->create_document(s);

            s = docname_base;
            s.append(string_t(postfix_minimal_, allocator_));
            mask_minimal_ = broker_->create_document(s);

            s = docname_base;
            s.append(string_t(postfix_complete_, allocator_));
            mask_complete_ = broker_->create_document(s);

            sensor_->template register_sensor_callback<self_type, &self_type::on_sens > (this);
            time_ = 0;

            debug_->debug("Init of sensor complete %s\n", uri.c_str());
        }

        void int_to_string_r(uint16_t i, string_t& r)
        {
            if (i > 10)
            {
                int_to_string_r(i / 10,r);
            }
            r.push_back('0' + (i % 10));
        }

        string_t int_to_string(int16_t i)
        {
            string_t r(allocator_);
            if (i < 0)
            {
                r.push_back('-');
                i = -i;
            }
            int_to_string_r(i, r);
            return r;
        }

        void on_sens(typename Sensor::value_t v)
        {
            // debug_->debug("sensing %s %d",observed_property_.c_str(), v);
            if(!busy_){
                last_measurement_ = v;
                timer_->template set_timer<self_type,
                     &self_type::update> (0, this, 0);
            }
        }

        void update(void* )
        {
            // debug_->debug("updating TS");
            busy_ = true;
            string_t date = int_to_string(time_);
            string_t data = int_to_string(last_measurement_);

            tuple_t t;
            bitmask_t old_bitmask_hasvalue, old_bitmask_date;

            t.set_allocator(allocator_);

            // Erase hasValue tuple for this sensor
            // ( $me :hasValue * * )

            t[COL_SUBJECT] = me_;
            t[COL_PREDICATE] = uri_hasValue;
            t.set_wildcard(COL_OBJECT, true);
            t.set_wildcard(COL_BITMASK, true);

            typename tuple_store_t::iterator it_end = tuple_store_->end( );
            /*
            for ( typename tuple_store_t::iterator it =  tuple_store_->begin( ); it != it_end; ++it )
            {
                tuple_t t;
                t.set_allocator( allocator_ );
                t[0] = (*it)[0];
                t[1] = (*it)[1];
                t[2] = (*it)[2];
                t[3] = (*it)[3];

                debug_->debug( "s %s p %s o %s m %s \n",t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ) );

            }
            */
            typename tuple_store_t::iterator iter = tuple_store_->find(t);
            if(iter != tuple_store_->end()) {
                old_bitmask_hasvalue = Broker::get_bitmask(*iter);
                 tuple_t t;
                t.set_allocator( allocator_ );
                t[0] = (*iter)[0];
                t[1] = (*iter)[1];
                t[2] = (*iter)[2];
                t[3] = (*iter)[3];
                // debug_->debug( "erasing s %s p %s o %s m %s \n",t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ) );
                // debug_->debug("pre erase");
                tuple_store_->erase(iter);
                /*
                debug_->debug("post erase");
                typename tuple_store_t::iterator it_end = tuple_store_->end( );
                for ( typename tuple_store_t::iterator it =  tuple_store_->begin( ); it != it_end; ++it )
				{
					tuple_t t;
					t.set_allocator( allocator_ );
					t[0] = (*it)[0];
					t[1] = (*it)[1];
					t[2] = (*it)[2];
					t[3] = (*it)[3];

					debug_->debug( "post erase 1 dump s %s p %s o %s m %s \n",t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ) );

				}
				*/
            }
            else {
                old_bitmask_hasvalue = 0;
            }
            t[COL_SUBJECT] = me_;
            t[COL_PREDICATE] = uri_date;
            t.set_wildcard(COL_OBJECT, true);
            t.set_wildcard(COL_BITMASK, true);


            iter = tuple_store_->find(t);
            if(iter != tuple_store_->end()) {
                old_bitmask_date = Broker::get_bitmask(*iter);

                tuple_store_->erase(iter);
            /*
                typename tuple_store_t::iterator it_end = tuple_store_->end( );
            for ( typename tuple_store_t::iterator it =  tuple_store_->begin( ); it != it_end; ++it )
            {
                tuple_t t;
                t.set_allocator( allocator_ );
                t[0] = (*it)[0];
                t[1] = (*it)[1];
                t[2] = (*it)[2];
                t[3] = (*it)[3];

                debug_->debug( "post erase 2 dump s %s p %s o %s m %s \n",t[0].c_str( ), t[1].c_str( ), t[2].c_str( ), t[3].c_str( ) );

            }
			*/
            }
            else {
                old_bitmask_date = 0;
            }

            // Insert new hasValue tuple
            // ( $me :hasValue * new_bitmask )

            t[COL_SUBJECT] = me_;
            t[COL_PREDICATE] = uri_hasValue;
            t[COL_OBJECT] = data;
            Broker::set_bitmask(t, old_bitmask_hasvalue | mask_intrinsic_ | mask_minimal_ | mask_complete_);

            tuple_store_->insert(t);

            t[COL_SUBJECT] = me_;
            t[COL_PREDICATE] = uri_date;
            t[COL_OBJECT] = date;
            Broker::set_bitmask(t, old_bitmask_hasvalue | mask_intrinsic_ | mask_minimal_ | mask_complete_);

            tuple_store_->insert(t);
            time_++;
            busy_ = false;
        }

    private:
        //typename iSenseTemperatureCallbackSensor<Os>::self_pointer_t sensor_;
        typename Sensor::self_pointer_t sensor_;
        typename Allocator::self_pointer_t allocator_;
        typename Broker::self_pointer_t broker_;
        typename Timer::self_pointer_t timer_;
        typename Debug_P::self_pointer_t debug_;
        static const char *postfix_intrinsic_,
                     *postfix_minimal_,
                     *postfix_complete_;
        bitmask_t mask_intrinsic_,
                  mask_minimal_,
                  mask_complete_;
        string_t me_, observed_property_, uom_;
        string_t uri_hasValue, uri_date;
        string_t uri_type, uri_observedProperty, uri_uomInUse, uri_sensor;
        uint16_t time_;
        typename Sensor::value_t last_measurement_;
        bool busy_;
    };


    template<typename OsModel_P,
    typename Broker_P,
    typename Sensor_P,
    typename Allocator_P,
    typename Debug_P,
    typename Timer_P
    >
    const char* SemanticDataProvider<OsModel_P, Broker_P, Sensor_P, Allocator_P,Debug_P,Timer_P>::postfix_intrinsic_ = "_intrinsic";

    template<typename OsModel_P,
    typename Broker_P,
    typename Sensor_P,
    typename Allocator_P,
    typename Debug_P,
    typename Timer_P
    >
    const char* SemanticDataProvider<OsModel_P, Broker_P, Sensor_P, Allocator_P,Debug_P,Timer_P >::postfix_minimal_ = "_minimal";

    template<typename OsModel_P,
    typename Broker_P,
    typename Sensor_P,
    typename Allocator_P,
    typename Debug_P,
    typename Timer_P
    >
    const char* SemanticDataProvider<OsModel_P, Broker_P, Sensor_P, Allocator_P,Debug_P,Timer_P >::postfix_complete_ = "_complete";
}

#endif	/* _SEMANTICDATAPROVIDER_H */

/* vim: set ts=4 sw=4 tw=78 expandtab :*/
