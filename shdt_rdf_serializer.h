#ifndef _SHDT_RDF_SERIALIZER_H
#define	_SHDT_RDF_SERIALIZER_H



#include "util/protobuf/varint.h"
#include "util/protobuf/string.h"


namespace wiselib
{

  template< typename OsModel_P, 
	    typename Iterator_P, 
	    typename Buffer_P,
	    typename Allocator_P,
	    typename String_P>

// 	    typename ID_P,
//   	    ID_P MAX_ID>
  class ShdtRdfSerializer
  {
    typedef Iterator_P iterator_t;
    typedef OsModel_P OsModel;
    typedef Buffer_P buffer_t;
    typedef Allocator_P allocator_t;
    typedef String_P sting_t;

    typedef uint8_t id_t; // todo: move to templ decl
    enum { TABLE_SIZE = 20 };  // todo: move to templ decl

    enum SubState
      { SUBS_SUBJECT=0, SUBS_WRITE_SUBJECT=1,
	SUBS_PREDICATE=2, SUBS_WRITE_PREDICATE=3,
	SUBS_OBJECT=4, SUBS_WRITE_OBJECT=5, 
	SUBS_TRIPLE=6, SUBS_HEADER=7 };

  public:
    ShdtRdfSerializer( iterator_t it,
		       iterator_t it_end,
		       typename allocator_t::self_pointer_t a )
    {
      it_ = it;
      it_end_ = it_end;
      substate_ = SUBS_HEADER;
    }

    ShdtRdfSerializer( typename allocator_t::self_pointer_t  )
    {
    }

    bool fill_buffer( buffer_t& buffer, 
		      size_t max_packet_size,
		      typename OsModel::Debug::self_pointer_t debug )
    {            
      debug->debug("fill %d %d\n", max_packet_size, (it_==it_end_)?1:0);
      if( max_packet_size==0 ) max_packet_size = (size_t)-1;

      debug->debug("xxx '%s'\n", (*it_)[0].c_str() );

      while( it_ != it_end_ )
	switch( substate_ )
	  {
	  case SUBS_HEADER:
	    if( buffer.position() + 2 + sizeof(id_t) > max_packet_size )
	      return true;

	    *buffer = sizeof(id_t); ++buffer;
	    *buffer = 0; ++buffer; // todo: add compression flags here
	    *buffer = TABLE_SIZE; ++buffer; // todo: should be a id_t, not a byte!
	    substate_ = SUBS_SUBJECT;
	    debug->debug("shdtheader %d %d %d // %s\n", sizeof(id_t), 0, TABLE_SIZE, (*it_)[0].c_str() );
	    break;
	  case SUBS_SUBJECT:
	  case SUBS_PREDICATE:
	  case SUBS_OBJECT:
	    {
	      bool needw;
	      collect_triple_[substate_/2] = 
		find_hash( & (*it_)[substate_/2],
			   substate_ == SUBS_SUBJECT ? TABLE_SIZE : collect_triple_[0],
			   substate_ == SUBS_OBJECT ? collect_triple_[1] : TABLE_SIZE,
			   needw );
	      substate_ += needw ? 1 : 2;
	    }
	    debug->debug("shdSPO %s %s %s\n", (*it_)[0].c_str(), (*it_)[1].c_str(), (*it_)[2].c_str() );
	    break;
	  case SUBS_WRITE_SUBJECT:
	  case SUBS_WRITE_PREDICATE:
	  case SUBS_WRITE_OBJECT:
	    {
	      if( buffer.position() + sizeof(id_t) + sizeof(id_t) + (*it_)[substate_/2].size() + 1 > max_packet_size
		  && buffer.position()!=0 ) // && !=0 just ensure to write when buffer is empty to avoid looping
		return true;
	      *buffer = (id_t)-1; ++buffer; // todo: should be id_t, not a byte
	      *buffer = collect_triple_[substate_/2]; ++buffer; // todo: should be id_t, not a byte
	      string_t ss = (*it_)[substate_/2];
	      string_t::Char* w = ss.c_str();//(*it_)[substate_/2].c_str();
	      debug->debug("shdtdict %d %d %d '%s' '%s'\n", (id_t)-1, collect_triple_[substate_/2], substate_, (const char*)w, (*it_)[0].c_str() );
	      do {
		*buffer=*w; ++buffer; ++w;
	      } while( *w != 0 );
	      
	      ++substate_;
	    }
	    break;
	  case SUBS_TRIPLE:
	    if( buffer.position() + sizeof(id_t)*3 > max_packet_size )
	      return true;
	    debug->debug( "shdttriple %d %d %d\n",
			  collect_triple_[0], collect_triple_[1], collect_triple_[2] );
	    *buffer = collect_triple_[0]; ++buffer; // todo: should be id_t, not a byte
	    *buffer = collect_triple_[1]; ++buffer; // todo: should be id_t, not a byte
	    *buffer = collect_triple_[2]; ++buffer; // todo: should be id_t, not a byte
	    ++it_; substate_=SUBS_SUBJECT;
	  };
      return false; //no more data
    }
    // ----------------------------------------------------------------------
    id_t
    find_hash( string_t* pt,
	       id_t avoid1, id_t avoid2,
	       bool& changed )
    {
      id_t base = hashvalue(pt);
      for( id_t off=0; off<3; ++off )
	if( hash_table_[ (base+off)%TABLE_SIZE ] == pt )
	  {
	    changed=false;
	    return (base+off)%TABLE_SIZE;
	  }

      id_t off = 0;
      while( (base+off)%TABLE_SIZE == avoid1 ||
	     (base+off)%TABLE_SIZE == avoid2 )
	++off;

      hash_table_[ (base+off)%TABLE_SIZE ] = pt;
      changed=true;
      return (base+off)%TABLE_SIZE;
    }
    // ----------------------------------------------------------------------
    id_t 
    hashvalue( string_t* pt )
    {
      return ( ( (unsigned int)pt )/sizeof(void*) )%TABLE_SIZE;
    }
    // --------------------------------------------------------------------
    void
    deserialize( typename OsModel::Radio::block_data_t *p,
		 typename OsModel::Radio::block_data_t *p_end,
		 typename OsModel::Debug::self_pointer_t debug) 
    {}  
    // --------------------------------------------------------------------
    void 
    deserialize( buffer_t& buf, 
		 typename OsModel::Debug::self_pointer_t debug )
    {}

  private:
    iterator_t it_;
    iterator_t it_end_;
    int substate_; // 0..2 check-dict 3 flush-trip

    string_t* hash_table_[TABLE_SIZE];
    id_t collect_triple_[3];
  };
}


#endif

