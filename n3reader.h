
#include <iostream>
#include <fstream>

#include <util/tuple_store/simple_tuple.h>

template<
	typename OsModel_P,
	typename Allocator_P,
	typename String_P,
	size_t COLUMNS_ = 3
>
class N3Reader {
	
		enum { MAX_LINE_LENGTH = 40960, COLUMNS = COLUMNS_, DELIMITER = '\t' };
	
		char current_line_[MAX_LINE_LENGTH];
		char *p_;
		std::ifstream stream_;
		char *data_[COLUMNS - 1];
		typename Allocator_P::self_pointer_t allocator_;
		
		std::map<unsigned char, size_t> read_chars_;
		size_t read_chars_sum_;
	
	public:
		
		typedef OsModel_P OsModel;
		typedef String_P string_t;
		typedef Allocator_P Allocator;
		
		typedef typename wiselib::SimpleTuple<OsModel, COLUMNS, string_t, Allocator> Tuple;
		
		N3Reader(const char* filename, typename Allocator::self_pointer_t allocator)
			: stream_(filename, std::ifstream::in), allocator_(allocator), read_chars_sum_(0) {
			//	printf("&alloc=%p\n", &allocator_);
			operator++();
		}
		
		void set_allocator(Allocator* alloc) {
		}
		
		bool printable(char c) {
			unsigned char uc = (unsigned char)c;
			return (uc >= 0x20) && (uc <= 0x7e);
		}
		
		bool whitespace(char c) {
			return (c == ' ') || (c == '\t') || (c == '\x0a') || (c == '\x0d');
		}
		
		int dehex(char c) {
			if(c >= '0' && c <= '9') { return c - '0'; }
			else if(c >= 'a' && c <= 'z') { return c - 'a' + 10; }
			else if(c >= 'A' && c <= 'Z') { return c - 'A' + 10; }
			else return 0;
		}
		
		void skip_whitespace() {
			for( ; whitespace(*p_); p_++) { }
		}
		
		void expect(char c) {
			assert(*p_ == c);
			p_++;
		}
		
		Tuple parse_line() {
			p_ = current_line_;
			
			Tuple r;
			r.set_allocator(allocator_);
			
			
			size_t column;
			string_t element;
			element.set_allocator(allocator_);
			
			skip_whitespace();
			for(column = 0; (column < COLUMNS) && *p_; column++) {
				r[column] = parse_element();
			}
			assert(column == COLUMNS);
			
			/*
			expect('.');
			expect('\0');
			*/
			return r;
		}
		
		string_t parse_element() {
			string_t r;
			r.set_allocator(allocator_);
			
			skip_whitespace();
			if(*p_ == '"') {
				r = parse_literal();
			}
			else if(*p_ == '<') {
				r = parse_uri();
			}
			else {
				r = parse_name();
			}
			skip_whitespace();
			return r;
		}
		
		string_t parse_lang() {
			string_t r(allocator_);
			
			assert(*p_ == '@');
			add_stats(*p_);
			r.push_back(*p_);
			r.append(parse_name());
			return r;
		}
		
		string_t parse_literal() {
			bool escaped = false;
			string_t r(allocator_);
			
			add_stats(*p_);
			r.push_back(*p_++);
			
			for( ; ; p_++) {
				if(escaped) {
					/*if(*p_ == 'u') {
						char u;
						for(int i=0; i<2; i++) {
							u = 16 * dehex(*p_++);
							u += dehex(*p_++);
							add_stats(u);
							r.push_back(u);
						}
					}
					else {*/
						add_stats(*p_);
						r.push_back(*p_);
					//}
					escaped = false;
				}
				else {
					if(*p_ == '\\') { escaped = true; }
					else {
						add_stats(*p_);
						r.push_back(*p_);
						if(*p_ == '"') {
							p_++;
							if(*p_ == '^') {
								add_stats(*p_); r.push_back(*p_++);
								assert(*p_ == '^');
								add_stats(*p_); r.push_back(*p_++);
								r.append(parse_element());
							}
							else if(*p_ == '@') {
								r.append(parse_lang());
							}
							break;
						}
					}
				}
			}
			return r;
		}
		
		string_t parse_uri() {
			string_t r(allocator_);
			
			add_stats(*p_);
			r.push_back(*p_++);
			
			for( ; ; p_++) {
				add_stats(*p_);
				r.push_back(*p_);
				if(*p_ == '>') {
					p_++;
					break;
				}
			}
			return r;
		}
		
		string_t parse_name() {
			string_t r(allocator_);
			
			for( ; *p_ && !whitespace(*p_) ; p_++) {
				add_stats(*p_);
				r.push_back(*p_);
			}
			return r;
		}
		
		/**
		 * Add character c to the history of output characters.
		 */
		void add_stats(char c) {
//			//assert(printable(c));
//			if(read_chars_.count(c) == 0) {
//				read_chars_[c] = 1;
//			}
//			else {
//				read_chars_[c]++;
//			}
//			read_chars_sum_++;
		}
		
		
		void print_stats() {
//			using namespace std;
//
//			cout << "# characters read: " << read_chars_sum_ << endl;
//
//			for(map<unsigned char, size_t>::iterator iter = read_chars_.begin(); iter != read_chars_.end(); ++iter) {
//				char c = iter->first;
//				float pct = (100.0 * iter->second / (float)read_chars_sum_);
//
//				cout << "'" << (printable(c) ? c : ' ') << "'"
//					<< " (0x" << hex << setw(2) << (int)(unsigned char)c << "):"
//					<< " " << dec << setw(6) << iter->second
//					<< " " << dec << fixed << setw(6)  << setprecision(2) << pct << "%"
//					<< "  ";
//
//				for(int i=0; i<pct*2; i++) {
//					cout << "*";
//				}
//				cout << endl;
//			}
		}
		
		N3Reader& operator++() {
			stream_.getline(current_line_, MAX_LINE_LENGTH);
			//printf("-- %u\n", strlen(current_line_));
			//printf("-- %s\n", current_line_);
			
			//current_line_[strlen(current_line_) - 3] = '\0';
			
			return *this;
		}
		
		/*
		string_t operator[](size_t idx) {
			if(idx == 0) {
				return parse_element(current_line_);
				//return string_t(current_line_ + 1, &allocator_);
			}
			else {
				return parse_element(data_[idx - 1]);
				//return string_t(data_[idx - 1], &allocator_);
			}
		}
		*/
		
		Tuple operator*() {
			/*int column = 0;
			for(char *p = current_line_; *p; p++) {
				if(*p == DELIMITER) {
					*p = '\0';
					data_[column] = p + 1;
					column++;
					if(column >= (COLUMNS - 1)) {
						break;
					}
				}
			}*/
			
			
			Tuple r(parse_line());
			return r;
		}
		
		//size_t size() { return COLUMNS; }
		
		//bool is_wildcard(size_t i) { return false; }
		
		bool ok() {
			return stream_;
		}
};

