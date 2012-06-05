/* 
 * File:   Serializer.h
 * Author: maxpagel
 *
 * Created on 8. Mai 2012, 15:34
 */

#ifndef _SERIALIZER_H
#define	_SERIALIZER_H

namespace wiselib{

    template<typename OsModel_P,typename Broker_P, typename Buffer_P>
    class RDFSerializer{
        typedef Broker_P broker_t;
        typedef OsModel_P OsModel;
        typedef Buffer_P buffer_t;

    public:
        RDFSerializer(broker_t::iterator& it, broker_t::iterator& it_end)
            :it_(it),it_end_(it_end)
        {

        }

        bool fill_buffer(buffer_t& buffer, size_t max_packet_size)
        {
            
        }

    private:
        broker_t::iterator& it_;
        broker_t::iterator& it_end_;
    };
}



#endif	/* _SERIALIZER_H */

