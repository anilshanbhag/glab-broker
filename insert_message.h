/* 
 * File:   insert_message.h
 * Author: maxpagel
 *
 * Created on 11. Mai 2012, 13:42
 */

#ifndef _INSERT_MESSAGE_H
#define	_INSERT_MESSAGE_H

#include "request_message.h"
namespace wiselib{

    template<typename OsModel_P,
            typename Radio_P = typename OsModel_P::Radio>
    class InsertMessage : public RequestMessage<OsModel_P,Radio_P> {

      typedef OsModel_P OsModel;
      typedef Radio_P Radio;
      typedef typename Radio::block_data_t block_data_t;
      typedef typename Radio::size_t size_t;
    public:
	  enum {
		  FLAG_COMPRESSED = 0x01
	  };

      inline void set_transaction_id( uint8_t id )
      { write<OsModel, block_data_t, uint8_t>( this->buffer + TRANSACTION_ID_POS, id ); }
      // --------------------------------------------------------------------
      inline uint8_t transaction_id()
      { return read<OsModel, block_data_t, uint8_t>( this->buffer + TRANSACTION_ID_POS ); }
      // --------------------------------------------------------------------
      inline void set_Insert_type( uint8_t id )
      { write<OsModel, block_data_t, uint8_t>( this->buffer + INSERT_TYPE_POS, id ); }
      // --------------------------------------------------------------------
      inline uint8_t insert_type()
      { return read<OsModel, block_data_t, uint8_t>( this->buffer + INSERT_TYPE_POS ); }
      // --------------------------------------------------------------------
      inline void set_type_action( uint8_t id )
      { write<OsModel, block_data_t, uint8_t>( this->buffer + TYPE_ACTION_POS, id ); }
      // --------------------------------------------------------------------
      inline uint8_t type_action()
      { return read<OsModel, block_data_t, uint8_t>( this->buffer + TYPE_ACTION_POS ); }
      // -------------------------------------------------------------------
      inline uint16_t payload_length()
      { return read<OsModel, block_data_t, uint16_t>( this->buffer + PAYLOAD_SIZE_POS ); }
      // --------------------------------------------------------------------
      inline block_data_t* payload()
      { return this->buffer + PAYLOAD_POS; }
      // --------------------------------------------------------------------
      inline void set_payload( size_t len, block_data_t *buf, size_t trlen=0 )
      {
         if(trlen == 0) { trlen = len; }
         set_payload_length( trlen );
         memcpy( this->buffer + PAYLOAD_POS, buf, len);
      }
      // --------------------------------------------------------------------
      inline size_t buffer_size()
      { return PAYLOAD_POS + payload_length(); }

      inline void set_payload_length( uint16_t len )
      { write<OsModel, block_data_t, uint16_t>( this->buffer + PAYLOAD_SIZE_POS, len ); }

	  uint8_t flags() { return read<OsModel, block_data_t, uint8_t>(this->buffer + PAYLOAD_FLAGS_POS); }
	  void set_flags(uint8_t f) {
		  write<OsModel, block_data_t, uint8_t>(this->buffer + PAYLOAD_FLAGS_POS, f);
	  }

private:
      enum positions
      {
         TRANSACTION_ID_POS = RequestMessage<OsModel_P,Radio_P>::REQUEST_TYPE + 1,
         INSERT_TYPE_POS = TRANSACTION_ID_POS + 1, //Command or Tuple
         TYPE_ACTION_POS = INSERT_TYPE_POS + 1, // 1 s 2p 3o 4document 5insert 6delete
		  
         PAYLOAD_FLAGS_POS = TYPE_ACTION_POS + 1,
        
         PAYLOAD_SIZE_POS = PAYLOAD_FLAGS_POS + 1,
         PAYLOAD_POS = PAYLOAD_SIZE_POS + 2
      };
    };
}

#endif	/* _INSERT_MESSAGE_H */

