/* 
 * File:   TransactionTuple.h
 * Author: maxpagel
 *
 * Created on 14. Mai 2012, 10:44
 */

#ifndef _TRANSACTIONTUPLE_H
#define	_TRANSACTIONTUPLE_H

#include "util/tuple_store/simple_tuple.h"


namespace wiselib{

    template<typename OsModel_P,
            typename String_P,
            typename Allocator_P,
            typename Broker_P >
    class TransactionTuple
        : public SimpleTuple<OsModel_P, 3, String_P, Allocator_P>{
    public:
        typedef OsModel_P OsModel;
        typedef String_P string_t;
        typedef Allocator_P allocator_t;
        typedef Broker_P broker_t;
        typedef typename broker_t::document_name_t document_name_t;
        typedef TransactionTuple<OsModel_P,String_P,Allocator_P,Broker_P> self_type;
        typedef typename allocator_t::template pointer_t<self_type> self_pointer_t;

    

        TransactionTuple(){
            state_ = 0;
        }

        TransactionTuple(typename broker_t::self_pointer_t broker,typename allocator_t::self_pointer_t allocator){
            state_ = 0;
            broker_ = broker;
            allocator_ = allocator;
            this->set_allocator(allocator_);
        }

        void init(typename broker_t::self_pointer_t broker,typename allocator_t::self_pointer_t allocator){
            state_ = 0;
            broker_ = broker;
            allocator_ = allocator;
            this->set_allocator(allocator_);
        }

        document_name_t document_name_;


        void set_subject(string_t& subject){
            (*this)[0]=subject;
            state_ |= 1 << 0;
        }

        void set_predicate(string_t& predicate){
            (*this)[1] = predicate;
            state_ |= 1 << 1;
        }

        void set_object(string_t& object){
            (*this)[2]=object;
            state_ |= 1 << 2;
        }

        void set_document_name(document_name_t& doc_name){
            //TODO bitmasken verodern erlauben
            document_name_=doc_name;
            state_ |= 1 << 3;
        }

        /**
         * 16 insert
         * 32 Delete
         */
        void set_command(uint8_t cmd){
            if(cmd==16 || cmd==32 || 64)
                state_ |= cmd;
        }
        bool execute_compressed(typename OsModel::Debug::self_pointer_t debug)
        {
			//debug->debug("exec compr");
		//	if(compressed) {
				if((state_ & (1 << 0)) && (state_ & (1 << 1)) && (state_ & (1 << 2)) && (state_ & (1 << 3)) ){
					if (state_ & (1 << 4))
					{//insert
						//debug->debug("trans compr insert\n");
						typename broker_t::bitmask_t bitmask = broker_->create_document(document_name_);
						broker_->template insert_compressed_tuple<self_type > (*this, bitmask);
						
						//for(typename broker_t::tuple_store_t::iterator it = broker_->tuple_store()->begin(); it != broker_->tuple_store()->end(); ++it) {
							//debug->debug("inserting (%s %s %s)\n",
									//(*it)[0].c_str(), (*it)[1].c_str(), (*it)[2].c_str());
						//}
						
						return true;
					} else if (state_ & (1 << 5)) //delete
					{
						typename broker_t::compressed_tuple_store_t::iterator it = broker_->template find_compressed_tuple<self_type > (*this);
						if (it != broker_->compressed_end())
							broker_->erase_compressed_tuple(it);
						return true;
					} else if (state_ & (1 << 6)) //log for Debug purpose
					{
						//debug->debug("tx compr complete %s %s %s %s\n", document_name_.c_str(), (*this)[0].c_str(), (*this)[1].c_str(), (*this)[2].c_str());
						return true;
					}
				}
			return false;
		}
		
		bool execute(typename OsModel::Debug::self_pointer_t debug) {
			//debug->debug("exec");
				if((state_ & (1 << 0)) && (state_ & (1 << 1)) && (state_ & (1 << 2)) && (state_ & (1 << 3)) ){
					if (state_ & (1 << 4))
					{//insert
						//debug->debug("trans insert");
						typename broker_t::bitmask_t bitmask = broker_->create_document(document_name_);
						broker_->template insert_tuple<self_type > (*this, bitmask);
						
						//for(typename broker_t::tuple_store_t::iterator it = broker_->tuple_store()->begin(); it != broker_->tuple_store()->end(); ++it) {
							//debug->debug("(%s %s %s)",
									//(*it)[0].c_str(), (*it)[1].c_str(), (*it)[2].c_str());
						//}
						
						return true;
					} else if (state_ & (1 << 5)) //delete
					{
						typename broker_t::tuple_store_t::iterator it = broker_->template find_tuple<self_type > (*this);
						if (it != broker_->end())
							broker_->erase_tuple(it);
						return true;
					} else if (state_ & (1 << 6)) //log for Debug purpose
					{
						//debug->debug("tx_complete %s %s %s %s\n", document_name_.c_str(), (*this)[0].c_str(), (*this)[1].c_str(), (*this)[2].c_str());
						return true;
					}
				}

            return false;
        }
    private:
        uint8_t state_;
        typename broker_t::self_pointer_t broker_;
        typename allocator_t::self_pointer_t allocator_;

    };
}

#endif	/* _TRANSACTIONTUPLE_H */

