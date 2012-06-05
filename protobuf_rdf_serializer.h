/* 
 * File:   protobuf_rdf_serializer.h
 * Author: maxpagel
 *
 * Created on 8. Mai 2012, 16:20
 */

#ifndef _PROTOBUF_RDF_SERIALIZER_H
#define	_PROTOBUF_RDF_SERIALIZER_H

#include "util/protobuf/message.h"

#include "util/protobuf/varint.h"
#include "util/protobuf/string.h"
//#include "util/protobuf/buffer_dynamic.h"

namespace wiselib
{

    template<typename OsModel_P, typename Iterator_P, typename Buffer_P, typename Allocator_P>
    class ProtobufRdfSerializer
    {
        typedef Iterator_P iterator_t;
        typedef OsModel_P OsModel;
        typedef Buffer_P buffer_t;
        typedef Allocator_P allocator_t;
        typedef unsigned int int_t;        

        typedef wiselib::protobuf::Message<OsModel, buffer_t, int_t> dynamic_message_t;
        typedef wiselib::protobuf::Message<OsModel, typename OsModel::Radio::block_data_t*, int_t> static_message_t;

    public:

        ProtobufRdfSerializer(iterator_t it, iterator_t it_end, typename allocator_t::self_pointer_t allocator)
        {
            it_ = it;
            it_end_ = it_end;
            allocator_ = allocator;
        }

        ProtobufRdfSerializer(typename allocator_t::self_pointer_t allocator)
        {
            allocator_ = allocator;
        }

        bool fill_buffer(buffer_t& buffer, size_t max_packet_size,typename OsModel::Debug::self_pointer_t debug)
        {            
            for (/*it_*/; it_ != it_end_; ++it_)
            {
                buffer_t statement(allocator_);
                dynamic_message_t::write(statement, 1, (*it_)[0]);
                
                dynamic_message_t::write(statement, 2, (*it_)[1]);
                
                dynamic_message_t::write(statement, 3, (*it_)[2]);
                
                
//                for(int i = 0; i<statement.size(); ++i){
//                   debug->debug("%c (%d)",statement.vector()[i],statement.vector()[i]);
//                }
//
//                debug->debug("\n");
                dynamic_message_t::write(buffer, 1, statement.vector());

            }
            return false; //no more data
        }

        void deserialize(typename OsModel::Radio::block_data_t *p,typename OsModel::Radio::block_data_t *p_end,typename OsModel::Debug::self_pointer_t debug) {
                
                int_t field;

//                for(int i = 0; i<=500; ++i){
//                   debug->debug("%d\n",p[i]);
//                }
                do {
                    buffer_dynamic_t statement(allocator_);
                    switch(static_message_t::field_number(p)) {
                        case 1:
                            static_message_t::read(p, field, statement.vector());
                            deserialize(statement,debug);
                            break;
                        default:
                            debug->debug("default");
                    }
                } while(p < p_end);
        }
// --------------------------------------------------------------------
        void deserialize(buffer_t& buf, typename OsModel::Debug::self_pointer_t debug) {
                int_t field;
                tuple_t t;
                t.set_allocator(allocator_);
                
                for(size_t i = 0; i<buf.size(); ++i){
                   debug->debug("%c",buf.vector()[i],buf.vector()[i]);
                }
                debug->debug("\n");
                do {
                    int_t fn = dynamic_message_t::field_number(buf);
                    string_t s(allocator_);
                    switch(fn) {
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
                        default:
                            {
                            debug->debug("%d\n", fn);
                            ++buf;
                            }
                    }
                        debug->debug( "output %s %s %s \n", t[0].c_str( ), t[1].c_str( ), t[2].c_str( ));
                } while(buf.readonly());
                debug->debug("deserializing statement done");
        }

    private:
        iterator_t it_;
        iterator_t it_end_;
        typename allocator_t::self_pointer_t allocator_;
    };
}


#endif	/* _PROTOBUF_RDF_SERIALIZER_H */

