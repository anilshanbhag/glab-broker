/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

/* 
 * File:   ssd_data_container.h
 * Author: maxpagel
 *
 * Created on 27. Dezember 2010, 16:29
 */

#ifndef _BROKER_H
#define _BROKER_H

#include "external_interface/default_return_values.h"
#include "util/delegates/delegate.hpp"
#include "config_testing.h"
#include <stdint.h>
#include "util/protobuf/message.h"

#include "util/protobuf/varint.h"
#include "util/protobuf/string.h"


#include <util/pstl/string_dynamic.h>
#include <util/pstl/BitString.h>
#include "util/protobuf/buffer_dynamic.h"
#include "util/tuple_store/simple_tuple.h"
#include "util/pstl/map_static_vector.h"
//#include <util/tuple_store/broker_tuple.h>
#include <util/tuple_store/int_dictionary.h>


namespace wiselib
{
    template<typename OsModel_P,            
            typename PlainTupleStore_P,
            typename Allocator_P,
            typename Bitmask_P,
            typename Debug_P = typename OsModel_P::Debug,
            typename Timer_P = typename OsModel_P::Timer>
   class Broker
   {
      
   public:
      typedef OsModel_P OsModel;        
      typedef Timer_P Timer;
      typedef Allocator_P Allocator_t;
      typedef Debug_P Debug;
      typedef Bitmask_P bitmask_t;
      typedef PlainTupleStore_P plain_tuple_store_t;
      typedef plain_tuple_store_t tuple_store_t;
      typedef typename plain_tuple_store_t::TupleStore compressed_tuple_store_t;
      typedef Broker<OsModel, plain_tuple_store_t, Allocator_t, bitmask_t, Debug, Timer> self_type;
      //typedef typename Allocator_t::template pointer_t<self_type> self_pointer_t;
      typedef self_type* self_pointer_t;
      typedef typename OsModel::block_data_t block_data_t;
      
        
      /*typedef wiselib::protobuf::VarInt<OsModel, OsModel::block_data_t*, unsigned int> varint_t;*/
      /*typedef wiselib::protobuf::String<OsModel, OsModel::block_data_t*, unsigned int> stringrw_t;*/
      typedef wiselib::string_dynamic<OsModel, Allocator_t> string_t;
      typedef wiselib::BitString_dynamic<OsModel, Allocator_t> bitstring_t;
      typedef string_t document_name_t;
      typedef uint16_t int_t;
        
      //typedef BrokerTuple<OsModel, bitmask_t, string_t, Allocator_t> plain_tuple_t;
      typedef typename plain_tuple_store_t::Tuple plain_tuple_t;
      typedef typename compressed_tuple_store_t::Tuple compressed_tuple_t;
      typedef plain_tuple_t tuple_t;
      
      typedef IntDictionary<OsModel, document_name_t, 8*sizeof(bitmask_t)> namedict_t;
      
      enum { MAX_SUBSCRIPTIONS = 5 };
      typedef delegate1<void, document_name_t> subscription_callback_t;
      typedef IntDictionary<OsModel, subscription_callback_t, MAX_SUBSCRIPTIONS> subscriptions_t;
      typedef typename subscriptions_t::key_type subscription_id_t;
      
   private:
      
      template<
         typename Store_P
      >
      class DocumentIterator {
         public:
            typedef Store_P Store;
            typedef typename Store::Tuple Tuple;
            
            DocumentIterator() {
            }
            
            DocumentIterator(const DocumentIterator& other) {
               *this = other;
            }
            
            DocumentIterator& operator=(const DocumentIterator& other) {
               documents_ = other.documents_;
               store_iterator_ = other.store_iterator_;
               store_iterator_end_ = other.store_iterator_end_;
               return *this;
            }
               
            DocumentIterator& operator++() {
               if(store_iterator_ != store_iterator_end_) {
                  ++store_iterator_;
               }
               forward();
               return *this;
            }
            
            Tuple operator*() {
               return *store_iterator_;
            }
            
            bool operator==(DocumentIterator other) {
               return store_iterator_ == other.store_iterator_;
            }
            
            bool operator!=(DocumentIterator other) {
               return store_iterator_ != other.store_iterator_;
            }
            
         
            DocumentIterator(bitmask_t documents,
                  typename Store::iterator store_iterator_begin,
                  typename Store::iterator store_iterator_end)
               : documents_(documents), store_iterator_(store_iterator_begin),
                     store_iterator_end_(store_iterator_end) {
               forward();
            }
         private:
            void forward() {
               // forward to next matching tuple
               //if(store_iterator_ == store_iterator_end_) {
                  //printf("fwd: start=end\n");
               //}
               //else {
                 //printf("fwd: bitmask=%x docs=%x\n", get_bitmask(*store_iterator_), documents_);
               //}
               
               return;
               while(
                     (store_iterator_ != store_iterator_end_) &&
                     !(get_bitmask(*store_iterator_) & documents_)) {
                  //printf("fwd: bitmask=%x docs=%x\n", get_bitmask(*store_iterator_), documents_);
                  ++store_iterator_;
               }
            }
            
            bitmask_t documents_;
            typename Store::iterator store_iterator_;
            typename Store::iterator store_iterator_end_;
      };
      
      static bitmask_t key_to_bitmask(typename namedict_t::key_type k) {
         return 1 << k;
      }
      
      static typename namedict_t::key_type bitmask_to_key(bitmask_t b) {
         typename namedict_t::key_type r = 0;
         while(b >>= 1) { r++; }
         return r;
      }

    public:
      typedef DocumentIterator<plain_tuple_store_t> iterator;
      typedef DocumentIterator<compressed_tuple_store_t> compressed_iterator;

      enum { COL_SUBJECT = 0, COL_PREDICATE = 1, COL_OBJECT = 2, COL_BITMASK = 3 };
      
        
        // --------------------------------------------------------------------
        Broker()
        : 
        timer_(0),
        debug_(0)
        {

        }
        // --------------------------------------------------------------------
        //       typename subscriptions_map_t::reference subscriptions()
        //       {
        //         return &subscriptions_;
        //       }
        // --------------------------------------------------------------------

        //void timeout_push(void*)
        //{

        //}
        // --------------------------------------------------------------------
      void init(Debug& debug, Timer& timer,
            typename plain_tuple_store_t::self_pointer_t plain_tuple_store,
            //typename compressed_tuple_store_t::self_pointer_t compressed_tuple_store,
            typename Allocator_t::self_pointer_t allocator)
      {
          
          timer_ = &timer;
          debug_ = &debug;
          plain_tuple_store_ = plain_tuple_store;
          compressed_tuple_store_ = plain_tuple_store_->tuple_store(); // compressed_tuple_store;
          plain_tuple_store_->set_ignore(0, false);
          plain_tuple_store_->set_ignore(1, false);
          plain_tuple_store_->set_ignore(2, false);
          plain_tuple_store_->set_ignore(3, true);
          allocator_ = allocator;
      }

      template<class T, void (T::*TMethod)(document_name_t)>
      subscription_id_t subscribe(T *obj_pnt)
      {
         return subscriptions_.insert(subscription_callback_t::template from_method<T, TMethod>(obj_pnt));
      }
      // --------------------------------------------------------------------
      void unsubscribe(subscription_id_t id) {
         subscriptions_.erase(id);
      }
      
      void document_has_changed(document_name_t name) {
         for(typename subscriptions_t::iterator iter = subscriptions_.begin(); iter != subscriptions_.end(); ++iter) {
            (*iter)(name);
         }
      }
      
// --------------------------------------------------------------------
      bitmask_t create_document(document_name_t name){
         //printf("---- create_document %s -----\n", name.c_str());
         
         typename namedict_t::key_type k = namedict_.find(name);
         if(k != namedict_t::NULL_KEY) {
            //printf("----- found\n");
            return key_to_bitmask(k);
         }
         else {
            //printf("----- NOT found\n");
            return key_to_bitmask(namedict_.insert(name));
         }
      }
      
      iterator begin_document(bitmask_t mask){
         //debug_->debug("begin_doc(%d)\n", mask);
         return iterator(mask, plain_tuple_store_->begin(), plain_tuple_store_->end());
      }
      iterator end_document(bitmask_t mask){
         return iterator(mask, plain_tuple_store_->end(), plain_tuple_store_->end());
      }
      iterator begin_document(document_name_t name){
         //debug_->debug("begin_doc(%s) -> %d -> %d\n", name.c_str(), namedict_.find(name), key_to_bitmask(namedict_.find(name)));
         return begin_document(key_to_bitmask(namedict_.find(name)));
      }
      iterator end_document(document_name_t name){
         return end_document(key_to_bitmask(namedict_.find(name)));
      }
      
      compressed_iterator begin_compressed_document(bitmask_t mask){
         return compressed_iterator(mask, compressed_tuple_store_->begin(), compressed_tuple_store_->end());
      }
      compressed_iterator end_compressed_document(bitmask_t mask){
         return compressed_iterator(mask, compressed_tuple_store_->end(), compressed_tuple_store_->end());
      }
      compressed_iterator begin_compressed_document(document_name_t name){
         return begin_compressed_document(key_to_bitmask(namedict_.find(name)));
      }
      compressed_iterator end_compressed_document(document_name_t name){
         return end_compressed_document(key_to_bitmask(namedict_.find(name)));
      }
      
// --------------------------------------------------------------------

      string_t get_document_name(bitmask_t mask) {
         return namedict_[bitmask_to_key(mask)];
      }
      
      template<typename Tuple>
      void insert_tuple(Tuple& tuple, bitmask_t mask) {
         plain_tuple_t t;
         t[COL_SUBJECT] = (tuple[COL_SUBJECT]);
         t[COL_PREDICATE] = (tuple[COL_PREDICATE]);
         t[COL_OBJECT] = (tuple[COL_OBJECT]);
         
         t[COL_BITMASK] = string_t(allocator_);
         t[COL_BITMASK].resize(sizeof(bitmask_t));
         set_bitmask(t, mask);
                  
         plain_tuple_store_->insert(t);
         //debug_->debug("done inserting");
      }
      
      template<typename Tuple>
      void insert_compressed_tuple(Tuple& tuple, bitmask_t mask) {
         compressed_tuple_t t;
         t[COL_SUBJECT] = (tuple[COL_SUBJECT]);
         t[COL_PREDICATE] = (tuple[COL_PREDICATE]);
         t[COL_OBJECT] = (tuple[COL_OBJECT]);
         
         t[COL_BITMASK] = bitstring_t(allocator_);
         t[COL_BITMASK].resize(8*sizeof(bitmask_t));
         set_bitmask(t, mask);
                  
         compressed_tuple_store_->insert(t);
      }
      
      /**
       * @ref query has 3 elements (subject, predicate and object)
       */
      template<typename Tuple>
      typename plain_tuple_store_t::iterator find_tuple(Tuple& query) {
         plain_tuple_t t;
         t[COL_SUBJECT] = (query[COL_SUBJECT]);
         t[COL_PREDICATE] = (query[COL_PREDICATE]);
         t[COL_OBJECT] = (query[COL_OBJECT]);
         t.set_wildcard(COL_BITMASK, true);
         return plain_tuple_store_->find(query);
      }
      
      template<typename Tuple>
      typename compressed_tuple_store_t::iterator find_compressed_tuple(Tuple& query) {
         compressed_tuple_t t;
         t[COL_SUBJECT] = (query[COL_SUBJECT]);
         t[COL_PREDICATE] = (query[COL_PREDICATE]);
         t[COL_OBJECT] = (query[COL_OBJECT]);
         t.set_wildcard(COL_BITMASK, true);
         return compressed_tuple_store_->find(query);
      }
      
      typename plain_tuple_store_t::iterator end() {
         return plain_tuple_store_->end();
      }
      
      typename compressed_tuple_store_t::iterator compressed_end() {
         return compressed_tuple_store_->end();
      }
        
      void erase_document(document_name_t name) {
         // TODO
      }
      
      //template<typename Iterator>
      void erase_tuple(typename plain_tuple_store_t::iterator iter) {
         plain_tuple_store_->erase(iter);
      }
      
      void erase_compressed_tuple(typename compressed_tuple_store_t::iterator iter) {
         compressed_tuple_store_->erase(iter);
      }
      
      // --------------------------------------------------------------------
      typename plain_tuple_store_t::self_pointer_t tuple_store(){
          return plain_tuple_store_;
      }
      // --------------------------------------------------------------------
      typename compressed_tuple_store_t::self_pointer_t compressed_tuple_store(){
          return compressed_tuple_store_;
      }
      // --------------------------------------------------------------------
      
      static bitmask_t get_bitmask(plain_tuple_t t) {
			bitmask_t bm;
         memcpy( (void*)&bm, (void*)(t[3].data()), sizeof(bitmask_t));
         return bm;
      }
      static bitmask_t get_bitmask(compressed_tuple_t t) {
			bitmask_t bm;
         memcpy( (void*)&bm, (void*)(t[3].data()), sizeof(bitmask_t));
         return bm;
      }
      
      static void set_bitmask(plain_tuple_t& t, bitmask_t bitmask) {
         t[3].resize(sizeof(bitmask_t));
         memcpy( (void*)t[3].data(), (void*)&bitmask, sizeof(bitmask_t));
      }
      static void set_bitmask(compressed_tuple_t& t, bitmask_t bitmask) {
         t[3].resize(sizeof(bitmask_t));
         memcpy( (void*)t[3].data(), (void*)&bitmask, sizeof(bitmask_t));
      }

   private:

        

      //       subscriptions_map_t subscriptions_;

      typename plain_tuple_store_t::self_pointer_t plain_tuple_store_;
      typename compressed_tuple_store_t::self_pointer_t compressed_tuple_store_;
      
      typename Allocator_t::self_pointer_t allocator_;
      namedict_t namedict_;
      Timer* timer_;
      Debug* debug_;
              
      subscriptions_t subscriptions_;

   };

}

#endif   /* _BROKER_H */

/* vim: set ts=3 sw=3 tw=78 expandtab :*/
