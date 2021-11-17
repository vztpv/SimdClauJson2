#pragma once



#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <iostream>
#include "simdjson.h" // modified simdjson 0.9.7

#include <map>
#include <vector>
#include <string>
#include <set>
#include <fstream>

namespace claujson {
	using STRING = std::string;

	class Data {
	public:
		simdjson::internal::tape_type type;

		bool is_key = false;

		union {
			long long int_val;
			unsigned long long uint_val;
			double float_val;
		};
		STRING str_val;

		Data(const Data& other)
			: type(other.type), int_val(other.int_val), str_val(other.str_val), is_key(other.is_key) {

		}

		Data(Data&& other)
			: type(other.type), int_val(other.int_val), str_val(std::move(other.str_val)), is_key(other.is_key) {

		}

		Data() : int_val(0), type(simdjson::internal::tape_type::ROOT) { }

		Data(simdjson::internal::tape_type _type, long long _number_val, STRING&& _str_val)
			:type(_type), int_val(_number_val), str_val(std::move(_str_val))
		{

		}

		bool operator==(const Data& other) const {
			if (this->type == other.type) {
				switch (this->type) {
				case simdjson::internal::tape_type::STRING:
					return this->str_val == other.str_val;
					break;
				}
			}
			return false;
		}

		bool operator<(const Data& other) const {
			if (this->type == other.type) {
				switch (this->type) {
				case simdjson::internal::tape_type::STRING:
					return this->str_val < other.str_val;
					break;
				}
			}
			return false;
		}

		Data& operator=(const Data& other) {
			if (this == &other) {
				return *this;
			}

			this->type = other.type;
			this->int_val = other.int_val;
			this->str_val = other.str_val;
			this->is_key = other.is_key;

			return *this;
		}


		Data& operator=(Data&& other) {
			if (this == &other) {
				return *this;
			}

			this->type = other.type;
			this->int_val = other.int_val;
			this->str_val = std::move(other.str_val);
			this->is_key = other.is_key;

			return *this;
		}
	};

	inline Data Convert(uint64_t* token, const std::unique_ptr<uint8_t[]>& string_buf) {
		uint8_t type = uint8_t((*token) >> 56);
		uint64_t payload = (*token) & simdjson::internal::JSON_VALUE_MASK;
		
		Data data;
		data.type = static_cast<simdjson::internal::tape_type>(type);
		
		uint32_t string_length;
		
		switch (type) {
		case '"': // we have a string
			std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
			data.str_val = std::string(
				reinterpret_cast<const char*>(string_buf.get() + payload + sizeof(uint32_t)),
				string_length
			);
			
			break;
		case 'l': // we have a long int
			data.int_val = *(token + 1);
			break;
		case 'u': // we have a long uint
			data.uint_val = *(token + 1);
			break;
		case 'd': // we have a double
			double answer;
			std::memcpy(&answer, token + 1, sizeof(answer));
			data.float_val = answer;
			break;
		case 'n': // we have a null
			break;
		case 't': // we have a true
			break;
		case 'f': // we have a false
			break;
		case '{': // we have an object
			break;    
		case '}': // we end an object
			break;
		case '[': // we start an array
			break;
		case ']': // we end an array
			break;
		case 'r':
			break;
		default:
			break;
		}

		return data;
	}

	class UserType {
	public:
		enum { Item, ArrayOrObject };
	private:
		UserType* make_user_type(const claujson::Data& name, int type) const {
			return new UserType(name, type);
		}

		UserType* make_user_type(int type) const {
			return new UserType(claujson::Data(), type);
		}

		UserType* make_user_type(claujson::Data&& name, int type) const {
			return new UserType(std::move(name), type);
		}

	public:
		void set_name(const STRING& name) {
			this->name.str_val = name;
		}

		bool is_user_type()const { return true; }
		bool is_item_type()const { return false; }
		UserType* clone() const {
			UserType* temp = new UserType(this->name);

			temp->type = this->type;

			temp->parent = nullptr; // chk!

			temp->data.reserve(this->data.size());
			temp->data2.reserve(this->data2.size());

			for (auto x : this->data) {
				temp->data.push_back(x->clone());
			}
			for (auto& x : this->data2) {
				temp->data2.push_back(x);
			}

			temp->order.reserve(this->order.size());

			for (auto x : this->order) {
				temp->order.push_back(x);
			}

			return temp;
		}

	private:
		claujson::Data name; // equal to key
		std::vector<UserType*> data; // ut data?
		std::vector<claujson::Data> data2; // it data
		int type = -1; // 0 - object, 1 - array, 2 - virtual object, 3 - virtual array, -1 - root  -2 - only in parse...
		UserType* parent = nullptr;

		std::vector<int> order; // ut data and it data order....
	public:
		inline const static size_t npos = -1; // ?
		// chk type?
		bool operator<(const UserType& other) const {
			return name.str_val < other.name.str_val;
		}
		bool operator==(const UserType& other) const {
			return name.str_val == other.name.str_val;
		}

	public:
		UserType(const UserType& other)
			: name(other.name),
			type(other.type), parent(other.parent)
		{
			this->data.reserve(other.data.size());
			for (auto& x : other.data) {
				this->data.push_back(x->clone());
			}

			this->data2.reserve(other.data2.size());
			for (auto& x : other.data2) {
				this->data2.push_back(x);
			}

			this->order.reserve(other.order.size());
			for (auto& x : other.order) {
				this->order.push_back(x);
			}
		}
		/*
		UserType(UserType&& other) {
			name = std::move(other.name);
			this->data = std::move(other.data);
			this->data2 = std::move(other.data2);
			type = std::move(other.type);
			parent = std::move(other.parent);
			order = std::move(other.order);
		}

		UserType& operator=(UserType&& other) noexcept {
			if (this == &other) {
				return *this;
			}

			name = std::move(other.name);
			data = std::move(other.data);
			data2 = std::move(other.data2);
			type = std::move(other.type);
			parent = std::move(other.parent);
			order = std::move(other.order);

			return *this;
		}
		*/
		const Data& get_name() const { return name; }


	private:
		void LinkUserType(UserType* ut) // friend?
		{
			data.push_back(ut);

			ut->parent = this;

			if (this->is_array()) {
				this->order.push_back(ArrayOrObject);
			}
		}
	private:
		UserType(claujson::Data&& name, int type = -1) : name(std::move(name)), type(type)
		{

		}

		UserType(const claujson::Data& name, int type = -1) : name(name), type(type)
		{
			//
		}
	public:
		UserType() : type(-1) {
			//
		}
		virtual ~UserType() {
			remove_all();
		}
	public:

		bool is_object() const {
			return type == 0 || type == 2;
		}

		bool is_array() const {
			return type == 1 || type == 3 || type  == -1 || type == -2;
		}

		bool is_in_root() const {
			return get_parent()->type == -1;
		}

		bool is_root() const {
			return type == -1;
		}

		static UserType* make_object(const claujson::Data& name) {
			UserType* ut = new UserType(name, 0);

			return ut;
		}

		static UserType* make_array(const claujson::Data& name) {
			UserType* ut = new UserType(name, 1);

			return ut;
		}

		// name key check?
		void add_object_element(const claujson::Data& name, const claujson::Data& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == 1) {
				throw "Error add object element to array in add_object_element ";
			}
			if (this->type == -1 && this->data.size() >= 1) {
				throw "Error not valid json in add_object_element";
			}

			this->data2.push_back(name);
			this->data2.push_back(data);

		}

		void add_array_element(const claujson::Data& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == 0) {
				throw "Error add object element to array in add_array_element ";
			}
			if (this->type == -1 && this->data.size() >= 1) {
				throw "Error not valid json in add_array_element";
			}

			this->data2.push_back(data); // (Type*)make_item_type(std::move(temp), data));

			this->order.push_back(Item);
		}

		void remove_all() {
			for (size_t i = 0; i < this->data.size(); ++i) {
				if (this->data[i]) {
					delete this->data[i];
					this->data[i] = nullptr;
				}
			}
			this->data.clear();
			this->data2.clear();
			this->order.clear();
		}

		void add_object_with_key(UserType* object) {
			const auto& name = object->name;

			if (is_array()) {
				throw "Error in add_object_with_key";
			}

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_object_with_key";
			}

			this->data.push_back(object);
			((UserType*)this->data.back())->parent = this;
		}

		void add_array_with_key(UserType* _array) {
			const auto& name = _array->name;

			if (is_array()) {
				throw "Error in add_array_with_key";
			}

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_array_with_key";
			}

			this->data.push_back(_array);
			((UserType*)this->data.back())->parent = this;
		}

		void add_object_with_no_key(UserType* object) {
			const Data& name = object->name;

			if (is_object()) {
				throw "Error in add_object_with_no_key";
			}

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_object_with_no_key";
			}

			this->data.push_back(object);
			((UserType*)this->data.back())->parent = this;

			order.push_back(ArrayOrObject);
		}

		void add_array_with_no_key(UserType* _array) {
			const Data& name = _array->name;

			if (is_object()) {
				throw "Error in add_array_with_no_key";
			}

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_array_with_no_key";
			}

			this->data.push_back(_array);
			((UserType*)this->data.back())->parent = this;

			order.push_back(ArrayOrObject);
		}

		void reserve_data_list(size_t len) {
			data.reserve(len);
		}

		void reserve_data2_list(size_t len) {
			data2.reserve(len);
		}


	private:

		void add_user_type(UserType* ut) {
			this->data.push_back(ut);
			ut->parent = this;

			if (!is_object()) {
				order.push_back(ArrayOrObject); // order need for "array"
			}
		}


		static UserType make_none() {
			Data temp;
			temp.str_val = "";
			temp.type = simdjson::internal::tape_type::STRING;

			UserType ut(temp, -2);

			return ut;
		}

		bool is_virtual() const {
			return type == 2 || type == 3;
		}

		static UserType make_virtual_object() {
			UserType ut;
			ut.type = 2;
			return ut;
		}

		static UserType make_virtual_array() {
			UserType ut;
			ut.type = 3;
			return ut;
		}

		void add_user_type(const Data& name, int type) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.
			// todo - chk this->type == -1 .. one object or one array or data(true or false or null or string or number).


			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_user_type";
			}

			this->data.push_back(make_user_type(name, type));

			((UserType*)this->data.back())->parent = this;
		}

		void add_user_type(int type) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.
			// todo - chk this->type == -1 .. one object or one array or data(true or false or null or string or number).


			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_user_type";
			}

			this->data.push_back(make_user_type(type));

			((UserType*)this->data.back())->parent = this;

			this->order.push_back(ArrayOrObject);
		}

		void add_user_type(Data&& name, int type) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.
			// todo - chk this->type == -1 .. one object or one array or data(true or false or null or string or number).

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_user_type";
			}

			this->data.push_back(make_user_type(std::move(name), type));
			((UserType*)this->data.back())->parent = this;
		}

		// add item_type in object? key = value
		void add_item_type(Data&& name, claujson::Data&& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_item_type";
			}

			{
				this->data2.push_back(name);
			}

			this->data2.push_back(std::move(data));
		}

		void add_item_type(claujson::Data&& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_item_type";
			}

			{
				this->order.push_back(Item);
			}

			this->data2.push_back(std::move(data));
		}

		void add_item_type(const Data& name, claujson::Data&& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_item_type";
			}


			this->data2.push_back(name);
			

			this->data2.push_back(std::move(data));
		}

		void add_item_type(const Data& name, const claujson::Data& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_item_type";
			}

			this->data2.push_back(name);

			this->data2.push_back(data);
		}

		void add_item_type(const claujson::Data& data) {
			// todo - chk this->type == 0 (object) but name is empty
			// todo - chk this->type == 1 (array) but name is not empty.

			if (this->type == -1 && this->data.size() + this->data2.size() >= 1) {
				throw "Error not valid json in add_item_type";
			}

			this->order.push_back(Item);

			this->data2.push_back(data);
		}
	public:
		UserType* get_data_list(const Data& name) {
			for (size_t i = this->data.size(); i > 0; --i) {
				if (name == this->data[i - 1]->get_name()) {
					return this->data[i - 1];
				}
			}
			throw "NOT EXIST in get_item_type_list";
		}
		const UserType* get_data_list(const Data& name) const {
			for (size_t i = this->data.size(); i > 0; --i) {
				if (name == this->data[i - 1]->get_name()) {
					return this->data[i - 1];
				}
			}
			throw "NOT EXIST in get_item_type_list";
		}

		UserType*& get_data_list(size_t idx) {
			return this->data[idx];
		}
		const UserType* const& get_data_list(size_t idx) const {
			return this->data[idx];
		}

		Data& get_data2_list(size_t idx) {
			return this->data2[idx];
		}
		const Data& get_data2_list(size_t idx) const {
			return this->data2[idx];
		}

		size_t get_data_size() const {
			return this->data.size();
		}

		size_t get_data2_size() const {
			return this->data2.size();
		}

		void remove_data_list(size_t idx) {
			delete data[idx];
			data.erase(data.begin() + idx);

			if (is_array() || is_root()) {
				size_t count = 0;
				for (size_t i = 0; i < order.size(); ++i) {
					if (is_array_or_object(i)) {
						if (idx == count) {
							order.erase(order.begin() + i);
							break;
						}
						count++;
					}
				}
			}
		}

		void remove_data2_list(size_t idx) {
			data2.erase(data2.begin() + idx);

			if (is_array() || is_root()) {
				size_t count = 0;
				for (size_t i = 0; i < order.size(); ++i) {
					if (is_item(i)) {
						if (idx == count) {
							order.erase(order.begin() + i);
							break;
						}
						count++;
					}
				}
			}
		}

		UserType* get_parent() {
			return parent;
		}

		const UserType* get_parent() const {
			return parent;
		}

		size_t get_order_size() const {
			return order.size();
		}

		void reserve_order_list(size_t len) {
			order.reserve(len);
		}

		bool is_item(size_t no) const {
			return order[no] == Item;
		}
		bool is_array_or_object(size_t no) const {
			return order[no] == ArrayOrObject;
		}

		friend class LoadData;
	};


	class LoadData
	{
	public:
		static int Merge(class UserType* next, class UserType* ut, class UserType** ut_next)
		{

			//check!!
			while (ut->get_data_size() >= 1
				&& (ut->get_data_list(0)->is_user_type()) && ((UserType*)ut->get_data_list(0))->is_virtual())
			{
				ut = (UserType*)ut->get_data_list(0);
			}

			bool chk_ut_next = false;

			while (true) {

				class UserType* _ut = ut;
				class UserType* _next = next;


				if (ut_next && _ut == *ut_next) {
					*ut_next = _next;
					chk_ut_next = true;
				}

				size_t _size = _ut->get_data_size(); // bug fix.. _next == _ut?
				for (size_t i = 0; i < _size; ++i) {
					if (((UserType*)_ut->get_data_list(i))->is_virtual()) {
						//_ut->get_user_type_list(i)->used();
					}
					else {
						_next->LinkUserType((UserType*)_ut->get_data_list(i));
						_ut->get_data_list(i) = nullptr;
					}
				}

				_size = _ut->get_data2_size();
				for (size_t i = 0; i < _size; ++i) {
					if (_ut->get_data2_list(i).is_key) { //  get_data2_list(0).is_key()) {
						_next->add_item_type(std::move(_ut->get_data2_list(i)), std::move(_ut->get_data2_list(i + 1)));
						++i;
					}
					else {
						_next->add_item_type(std::move(_ut->get_data2_list(i)));
					}
				}

				_ut->remove_all();

				ut = ut->get_parent();
				next = next->get_parent();


				if (next && ut) {
					//
				}
				else {
					// right_depth > left_depth
					if (!next && ut) {
						return -1;
					}
					else if (next && !ut) {
						return 1;
					}

					return 0;
				}
			}
		}

	private:
		static bool __LoadData(const std::unique_ptr<uint8_t[]>& string_buf, const std::unique_ptr<uint64_t[]>& token_arr,
			int64_t token_arr_start, size_t token_arr_len, const std::vector<int>& is_key, class UserType* _global,
			int start_state, int last_state, class UserType** next, int* err, int no)
		{

			std::vector<Data> Vec;

			if (token_arr_len <= 0) {
				*next = nullptr;
				return false;
			}

			class UserType& global = *_global;

			int state = start_state;
			size_t braceNum = 0;
			std::vector< class UserType* > nestedUT(1);

			nestedUT.reserve(10);
			nestedUT[0] = &global;

			int64_t count = 0;

			Data key;

			for (int64_t i = 0; i < token_arr_len; ++i) {

				const simdjson::internal::tape_type type = 
					static_cast<simdjson::internal::tape_type>((((token_arr)[token_arr_start + i]) >> 56));
				
				//std::cout << (int)type << "\n";

				switch (state)
				{
				case 0:
				{
					// Left 1
					if (type == simdjson::internal::tape_type::START_OBJECT ||
						type == simdjson::internal::tape_type::START_ARRAY) { // object start, array start

						if (!Vec.empty()) {
							
							if (Vec[0].is_key) { // with key
								nestedUT[braceNum]->reserve_data2_list(nestedUT[braceNum]->get_data2_size() + Vec.size());
							}
							else {
								nestedUT[braceNum]->reserve_data2_list(nestedUT[braceNum]->get_data2_size() + Vec.size());
								nestedUT[braceNum]->reserve_order_list(nestedUT[braceNum]->get_order_size() + Vec.size());
							}

							if (Vec[0].is_key) {
								for (size_t x = 0; x < Vec.size(); x += 2) {
									nestedUT[braceNum]->add_item_type(std::move(Vec[x]), std::move(Vec[x + 1]));
								}
							}
							else {
								for (size_t x = 0; x < Vec.size(); x += 1) {
									nestedUT[braceNum]->add_item_type(std::move(Vec[x]));
								}
							}

							Vec.clear();
						}

						if (key.is_key) {
							nestedUT[braceNum]->add_user_type(std::move(key), type == simdjson::internal::tape_type::START_OBJECT ? 0 : 1); // object vs array
							key = Data();
						}
						else {
							nestedUT[braceNum]->add_user_type(type == simdjson::internal::tape_type::START_OBJECT? 0 : 1);
						}
						class UserType* pTemp = (UserType*)(((UserType*)nestedUT[braceNum])->get_data_list(((UserType*)nestedUT[braceNum])->get_data_size() - 1));

						braceNum++;

						/// new nestedUT
						if (nestedUT.size() == braceNum) {
							nestedUT.push_back(nullptr);
						}

						/// initial new nestedUT.
						nestedUT[braceNum] = pTemp;

						state = 0;

					}
					// Right 2
					else if (type == simdjson::internal::tape_type::END_OBJECT ||
						type == simdjson::internal::tape_type::END_ARRAY) {

						state = 0;

						if (!Vec.empty()) {
							if (type == simdjson::internal::tape_type::END_OBJECT) {
								nestedUT[braceNum]->reserve_data2_list(nestedUT[braceNum]->get_data2_size() + Vec.size());
								
								for (size_t x = 0; x < Vec.size(); x += 2) {
									nestedUT[braceNum]->add_item_type(std::move(Vec[x]), std::move(Vec[x + 1]));
								}
							}
							else { // END_ARRAY
								nestedUT[braceNum]->reserve_data2_list(nestedUT[braceNum]->get_data2_size() + Vec.size());
								nestedUT[braceNum]->reserve_order_list(nestedUT[braceNum]->get_order_size() + Vec.size());

								for (size_t x = 0; x < Vec.size(); x += 1) {
									nestedUT[braceNum]->add_item_type(std::move(Vec[x]));
								}
							}

							Vec.clear();
						}


						if (braceNum == 0) {
							class UserType ut; //
							
							ut.add_user_type(type == simdjson::internal::tape_type::END_OBJECT ? 2 : 3); // json -> "var_name" = val  

							for (size_t i = 0; i < nestedUT[braceNum]->get_data_size(); ++i) {
								((UserType*)ut.get_data_list(0))->add_user_type((UserType*)(nestedUT[braceNum]->get_data_list(i)));
								nestedUT[braceNum]->get_data_list(i) = nullptr;
							}

							for (size_t i = 0; i < nestedUT[braceNum]->get_data2_size(); ++i) {
								if (type == simdjson::internal::tape_type::END_OBJECT) { //nestedUT[braceNum]->get_data2_list(0).is_key()) {
									((UserType*)ut.get_data_list(0))->add_item_type(std::move((nestedUT[braceNum]->get_data2_list(i))),
										std::move((nestedUT[braceNum]->get_data2_list(i + 1))));
									++i;
								}
								else {
									((UserType*)ut.get_data_list(0))->add_item_type(std::move((nestedUT[braceNum]->get_data2_list(i))));
								}
							}


							nestedUT[braceNum]->remove_all();
							nestedUT[braceNum]->add_user_type((UserType*)(ut.get_data_list(0)));

							ut.get_data_list(0) = nullptr;

							braceNum++;
						}
						
						{						
							if (braceNum < nestedUT.size()) {
								nestedUT[braceNum] = nullptr;
							}

							braceNum--;
						}
					}
					else {
						{

							Data data = Convert(&(token_arr[token_arr_start + i]), string_buf);
							
							if (is_key[token_arr_start + i]) {
								data.is_key = true;

								key = data;

								const simdjson::internal::tape_type _type =
									static_cast<simdjson::internal::tape_type>((((token_arr)[token_arr_start + i + 1]) >> 56));
								
								
								if (_type == simdjson::internal::tape_type::START_ARRAY || _type == simdjson::internal::tape_type::START_OBJECT) {
									//
								}
								else {
									Vec.push_back(data);
								}
							}
							else {	//std::cout << data.str_val << " \n ";
								Vec.push_back(data);
							}
							
							state = 0;
						}	
					}
				}
				break;
				default:
					// syntax err!!
					*err = -1;
					return false; // throw "syntax error ";
					break;
				}
			
				switch ((int)type) {
				case '"': // we have a string
					//os << "string \"";
				   // std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
				   // os << internal::escape_json_string(std::string_view(
				   //     reinterpret_cast<const char*>(string_buf.get() + payload + sizeof(uint32_t)),
				   //     string_length
				   // ));
				   // os << '"';
				  //  os << '\n';
					
					break;
				case 'l': // we have a long int
					++i;

					break;
				case 'u': // we have a long uint
					++i;
					break;
				case 'd': // we have a double
					++i;
					break;
				case 'n': // we have a null
				   // os << "null\n";
					break;
				case 't': // we have a true
				   // os << "true\n";
					break;
				case 'f': // we have a false
				  //  os << "false\n";
					break;
				case '{': // we have an object
				 //   os << "{\t// pointing to next tape location " << uint32_t(payload)
				 //       << " (first node after the scope), "
				  //      << " saturated count "
				   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";

					
					break;
				case '}': // we end an object
				  //  os << "}\t// pointing to previous tape location " << uint32_t(payload)
				  //      << " (start of the scope)\n";
					
					break;
				case '[': // we start an array
				  //  os << "[\t// pointing to next tape location " << uint32_t(payload)
				  //      << " (first node after the scope), "
				  //      << " saturated count "
				   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
					
					break;
				case ']': // we end an array
				 //   os << "]\t// pointing to previous tape location " << uint32_t(payload)
				  //      << " (start of the scope)\n";
				
					break;
				case 'r': // we start and end with the root node
				  // should we be hitting the root node?
					break;
				default:

					break;
				}
			}

			if (next) {
				*next = nestedUT[braceNum];
			}

			if (Vec.empty() == false) {
				if (Vec[0].is_key) {
					for (size_t x = 0; x < Vec.size(); x += 2) {
						nestedUT[braceNum]->add_item_type(std::move(Vec[x]), std::move(Vec[x + 1]));
					}
				}
				else {
					for (size_t x = 0; x < Vec.size(); x += 1) {
						nestedUT[braceNum]->add_item_type(std::move(Vec[x]));
					}
				}

				Vec.clear();
			}

			if (state != last_state) {
				*err = -2;
				return false;
				// throw STRING("error final state is not last_state!  : ") + toStr(state);
			}

			return true;
		}

		// need random access..
		static int64_t FindDivisionPlace(const std::unique_ptr<uint64_t[]>& token_arr, int64_t start, int64_t last)
		{
			for (int64_t a = start; a <= last; ++a) {
				auto& x = token_arr[a];
				const simdjson::internal::tape_type type = static_cast<simdjson::internal::tape_type>(x >> 56);

				int64_t A = a;

				switch ((int)type) {
				case '"': // we have a string
					//os << "string \"";
				   // std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
				   // os << internal::escape_json_string(std::string_view(
				   //     reinterpret_cast<const char*>(string_buf.get() + payload + sizeof(uint32_t)),
				   //     string_length
				   // ));
				   // os << '"';
				  //  os << '\n';
					break;
				case 'l': // we have a long int
					
					//  os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
					++a;
					break;
				case 'u': // we have a long uint
					
					//  os << "unsigned integer " << tape[++tape_idx] << "\n";
					++a;
					break;
				case 'd': // we have a double
				  //  os << "float ";
					
					//double answer;
					// std::memcpy(&answer, &tape[++tape_idx], sizeof(answer));
				   //  os << answer << '\n';
					++a;
					break;
				case 'n': // we have a null
				   // os << "null\n";
					break;
				case 't': // we have a true
				   // os << "true\n";
					break;
				case 'f': // we have a false
				  //  os << "false\n";
					break;
				case '{': // we have an object
				 //   os << "{\t// pointing to next tape location " << uint32_t(payload)
				 //       << " (first node after the scope), "
				  //      << " saturated count "
				   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
					break;
				case '}': // we end an object
				  //  os << "}\t// pointing to previous tape location " << uint32_t(payload)
				  //      << " (start of the scope)\n";
					break;
				case '[': // we start an array
				  //  os << "[\t// pointing to next tape location " << uint32_t(payload)
				  //      << " (first node after the scope), "
				  //      << " saturated count "
				   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
					break;
				case ']': // we end an array
				 //   os << "]\t// pointing to previous tape location " << uint32_t(payload)
				  //      << " (start of the scope)\n";
					break;
				case 'r': // we start and end with the root node
				  // should we be hitting the root node?
					break;
				default:
					// error?
					break;
				}


				if (type == simdjson::internal::tape_type::START_OBJECT || type == simdjson::internal::tape_type::START_ARRAY) { // left
					return A + 1;
				}
			}
			return -1;
		}
	public:

		static bool _LoadData(class UserType& global, const std::unique_ptr<uint8_t[]>& string_buf, const std::unique_ptr<uint64_t[]>& token_arr, int64_t& length,
			const std::vector<int>& key, std::vector<int64_t>& start, const int parse_num) // first, strVec.empty() must be true!!
		{
			const int pivot_num = parse_num - 1;
			//size_t token_arr_len = length; // size?

			class UserType* before_next = nullptr;
			class UserType _global;

			bool first = true;
			int64_t sum = 0;

			{
				std::set<int64_t> _pivots;
				std::vector<int64_t> pivots;
				//const int64_t num = token_arr_len; //

				if (pivot_num > 0) {
					std::vector<int64_t> pivot;
					pivots.reserve(pivot_num);
					pivot.reserve(pivot_num);

					pivot.push_back(start[0]);

					for (int i = 1; i < parse_num; ++i) {
						pivot.push_back(FindDivisionPlace(token_arr, start[i], start[i + 1] - 1));
					}

					for (size_t i = 0; i < pivot.size(); ++i) {
						if (pivot[i] != -1) {
							_pivots.insert(pivot[i]);
						}
					}

					for (auto& x : _pivots) {
						pivots.push_back(x);
					}

					pivots.push_back(length - 1);
				}

				std::vector<class UserType*> next(pivots.size() - 1, nullptr);

				{
					std::vector<class UserType> __global(pivots.size() - 1, UserType::make_none());

					std::vector<std::thread> thr(pivots.size() - 1);
					std::vector<int> err(pivots.size() - 1, 0);
					{
						int64_t idx = pivots.size() < 2 ? length - 1 : pivots[1] - pivots[0];
						int64_t _token_arr_len = idx;

						thr[0] = std::thread(__LoadData, std::ref(string_buf), std::ref(token_arr), start[0], _token_arr_len, std::ref(key), &__global[0], 0, 0, &next[0], &err[0], 0);
					}

					for (size_t i = 1; i < pivots.size() - 1; ++i) {
						int64_t _token_arr_len = pivots[i + 1] - pivots[i];

						thr[i] = std::thread(__LoadData, std::ref(string_buf), std::ref(token_arr), pivots[i], _token_arr_len, std::ref(key), &__global[i], 0, 0, &next[i], &err[i], i);
					}


					auto a = std::chrono::steady_clock::now();

					// wait
					for (size_t i = 0; i < thr.size(); ++i) {
						thr[i].join();
					}


					auto b = std::chrono::steady_clock::now();
					auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
					std::cout << "parse1 " << dur.count() << "ms\n";

					for (size_t i = 0; i < err.size(); ++i) {
						switch (err[i]) {
						case 0:
							break;
						case -1:
						case -4:
							std::cout << "Syntax Error\n"; return false;
							break;
						case -2:
							std::cout << "error final state is not last_state!\n"; return false;
							break;
						case -3:
							std::cout << "error x > buffer + buffer_len:\n"; return false;
							break;
						default:
							std::cout << "unknown parser error\n"; return false;
							break;
						}
					}

					// Merge
					//try
					{
						int i = 0;
						std::vector<int> chk(8, 0);
						auto x = next.begin();
						auto y = __global.begin();
						while (true) {
							if (y->get_data_size() + y->get_data2_size() == 0) {
								chk[i] = 1;
							}

							++x;
							++y;
							++i;

							if (x == next.end()) {
								break;
							}
						}

						int start = 0;
						int last = pivots.size() - 1 - 1;

						for (int i = 0; i < pivots.size() - 1; ++i) {
							if (chk[i] == 0) {
								start = i;
								break;
							}
						}

						for (int i = pivots.size() - 1 - 1; i >= 0; --i) {
							if (chk[i] == 0) {
								last = i;
								break;
							}
						}

						if (__global[start].get_data_size() > 0 && __global[start].get_data_list(0)->is_user_type()
							&& ((UserType*)__global[start].get_data_list(0))->is_virtual()) {
							std::cout << "not valid file1\n";
							throw 1;
						}
						if (next[last] && next[last]->get_parent() != nullptr) {
							std::cout << "not valid file2\n";
							throw 2;
						}



						int err = Merge(&_global, &__global[start], &next[start]);
						if (-1 == err || (pivots.size() == 0 && 1 == err)) {
							std::cout << "not valid file3\n";
							throw 3;
						}

						for (int i = start + 1; i <= last; ++i) {

							if (chk[i]) {
								continue;
							}

							// linearly merge and error check...
							int before = i - 1;
							for (int k = i - 1; k >= 0; --k) {
								if (chk[k] == 0) {
									before = k;
									break;
								}
							}
							
							int err = Merge(next[before], &__global[i], &next[i]);

							if (-1 == err) {
								std::cout << "chk " << i << " " << __global.size() << "\n";
								std::cout << "not valid file4\n";
								throw 4;
							}
							else if (i == pivots.size() && 1 == err) {					
								std::cout << "not valid file5\n";
								throw 5;
							}
						}
					}
					//catch (...) {
						//throw "in Merge, error";
					//	return false;
					//}
					//
					before_next = next.back();

					auto c = std::chrono::steady_clock::now();
					auto dur2 = std::chrono::duration_cast<std::chrono::nanoseconds>(c - b);
					std::cout << "parse2 " << dur2.count() << "ns\n";
				}
			}
			//int a = clock();

			Merge(&global, &_global, nullptr);

			/// global = std::move(_global);
			//int b = clock();
			//std::cout << "chk " << b - a << "ms\n";
			return true;
		}
		static bool parse(class UserType& global, const std::unique_ptr<uint8_t[]>& string_buf, const std::unique_ptr<uint64_t[]>& tokens, int64_t length, const std::vector<int>& key, std::vector<int64_t>& start, int thr_num) {
			return LoadData::_LoadData(global, string_buf, tokens, length, key, start, thr_num);
		}

		//
		static void _save(std::ostream& stream, UserType* ut, const int depth = 0) {

			for (size_t i = 0; i < ut->get_data2_size(); ++i) {
				auto& x = ut->get_data2_list(i);

				if (
					x.type == simdjson::internal::tape_type::STRING) {
					stream << "\"";
					for (long long j = 0; j < x.str_val.size(); ++j) {
						switch (x.str_val[j]) {
						case '\\':
							stream << "\\\\";
							break;
						case '\"':
							stream << "\\\"";
							break;
						case '\n':
							stream << "\\n";
							break;

						default:
							if (isprint(x.str_val[j]))
							{
								stream << x.str_val[j];
							}
							else
							{
								int code = x.str_val[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf + 2, "%04X", code);
									stream << buf;
								}
								else {
									stream << x.str_val[j];
								}
							}
						}
					}

					stream << "\"";

					if (x.is_key) {
						stream << " : ";
					}
				}
				else if (x.type == simdjson::internal::tape_type::TRUE_VALUE) {
					stream << "true";
				}
				else if (x.type == simdjson::internal::tape_type::FALSE_VALUE) {
					stream << "false";
				}
				else if (x.type == simdjson::internal::tape_type::DOUBLE) {
					stream << (x.float_val);
				}
				else if (x.type == simdjson::internal::tape_type::INT64) {
					stream << x.int_val;
				}
				else if (x.type == simdjson::internal::tape_type::UINT64) {
					stream << x.uint_val;
				}
				else if (x.type == simdjson::internal::tape_type::NULL_VALUE) {
					stream << "null ";
				}

				if (ut->is_object() && i % 2 == 1 && i < ut->get_data2_size() - 1) {
					stream << ",";
				}
				else if (ut->is_array() && i < ut->get_data2_size() - 1) {
					stream << ",";
				}
				stream << " ";

			}


			if (ut->get_data2_size() > 0 && ut->get_data_size() > 0) {
				stream << ",\n";
			}

			for (size_t i = 0; i < ut->get_data_size(); ++i) {

				if (ut->is_object()) {
					stream << "\"" << ut->get_data_list(i)->get_name().str_val << "\" : ";
				}

				{ // ut is array
					if (((UserType*)ut->get_data_list(i))->is_object()) {
						stream << " { \n";
					}
					else {
						stream << " [ \n";
					}
				}

				_save(stream, (UserType*)ut->get_data_list(i), depth + 1);

				if (((UserType*)ut->get_data_list(i))->is_object()) {
					stream << " } \n";
				}
				else {
					stream << " ] \n";
				}


				if (i < ut->get_data_size() - 1) {
					stream << ",";
				}
			}
		}

		static void save(const std::string& fileName, class UserType& global) {
			std::ofstream outFile;
			outFile.open(fileName, std::ios::binary); // binary!

			_save(outFile, &global);

			outFile.close();
		}
	};
	
	inline int Parse(const std::string& fileName, int thr_num, UserType* ut)
	{
		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}



		int _ = clock();

		{
			simdjson::dom::parser test;

			auto x = test.load(fileName);

			if (x.error() != simdjson::error_code::SUCCESS) {
				std::cout << x.error() << "\n";

				return -1;
			}

			const auto& tape = test.raw_tape();
			const auto& string_buf = test.raw_string_buf();

			std::vector<int64_t> start(thr_num + 1, 0);
			std::vector<int> key;
			int64_t length;
			int a = clock();

			std::cout << a - _ << "ms\n";
			{
				uint32_t string_length;
				size_t tape_idx = 0;
				uint64_t tape_val = tape[tape_idx];
				uint8_t type = uint8_t(tape_val >> 56);
				uint64_t payload;
				tape_idx++;
				size_t how_many = 0;
				if (type == 'r') {
					how_many = size_t(tape_val & simdjson::internal::JSON_VALUE_MASK);
					length = how_many;

					key = std::vector<int>(how_many, 0);
				}
				else {
					// Error: no starting root node?
					return -1;
				}

				start[0] = 1;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}

				// remove?
				//int expect_depth_max = 10;
				//std::vector<int> _stack; _stack.reserve(expect_depth_max);
				//std::vector<int> _stack2; _stack2.reserve(expect_depth_max);

				int c = clock();
	
				int count = 1;
				for (; tape_idx < how_many; tape_idx++) {
					if (count < thr_num && tape_idx == start[count]) {
						count++; 
					}
					else if (count < thr_num && tape_idx == start[count] + 1) {
						start[count] = tape_idx;
						count++; 
					}

					tape_val = tape[tape_idx];
					payload = tape_val & simdjson::internal::JSON_VALUE_MASK;
					type = uint8_t(tape_val >> 56);

					switch (type) {
					case 'l':
					case 'u':
					case 'd':
						tape_idx++;
						break;
					}
				}

				
				int d = clock();
				std::cout << d - c << "ms\n";
				bool now_object = false;
				bool even = false;

				for (tape_idx = 1; tape_idx < how_many; tape_idx++) {
					//os << tape_idx << " : ";
					tape_val = tape[tape_idx];
					payload = tape_val & simdjson::internal::JSON_VALUE_MASK;
					type = uint8_t(tape_val >> 56);
					
					even = !even;

					switch (type) {
					case '"': // we have a string
						if (now_object && even) {
							key[tape_idx] = 1;
						}

						break;
					case 'l': // we have a long int
					//	if (tape_idx + 1 >= how_many) {
					//		return false;
						//}
						//  os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
						++tape_idx;

						break;
					case 'u': // we have a long uint
						//if (tape_idx + 1 >= how_many) {
						//	return false;
						//}
						//  os << "unsigned integer " << tape[++tape_idx] << "\n";
						++tape_idx;
						break;
					case 'd': // we have a double
					  //  os << "float ";
						//if (tape_idx + 1 >= how_many) {
						//	return false;
						//}

						// double answer;
						// std::memcpy(&answer, &tape[++tape_idx], sizeof(answer));
					   //  os << answer << '\n';
						++tape_idx;
						break;
					case 'n': // we have a null
					   // os << "null\n";
						break;
					case 't': // we have a true
					   // os << "true\n";
						break;
					case 'f': // we have a false
					  //  os << "false\n";
						break;
					case '{': // we have an object
					 //   os << "{\t// pointing to next tape location " << uint32_t(payload)
					 //       << " (first node after the scope), "
					  //      << " saturated count "
					   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
						now_object = true; even = false;
						//_stack.push_back(1);
						//_stack2.push_back(0);
						break;
					case '}': // we end an object
					  //  os << "}\t// pointing to previous tape location " << uint32_t(payload)
					  //      << " (start of the scope)\n";
						//_stack.pop_back();
						//_stack2.pop_back();

						now_object = key[uint32_t(payload) - 1] == 1; even = false;
						break;
					case '[': // we start an array
					  //  os << "[\t// pointing to next tape location " << uint32_t(payload)
					  //      << " (first node after the scope), "
					  //      << " saturated count "
					   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
						//_stack.push_back(0);
						now_object = false; even = false;
						break;
					case ']': // we end an array
					 //   os << "]\t// pointing to previous tape location " << uint32_t(payload)
					  //      << " (start of the scope)\n";
						//_stack.pop_back();
						now_object = key[uint32_t(payload) - 1] == 1; even = false;
						break;
					case 'r': // we start and end with the root node
					  // should we be hitting the root node?
						break;
					default:

						return -3;
					}
				}
				std::cout << clock() - d << "ms\n";
			}

			int b = clock();

			std::cout << b - a << "ms\n";

			start[thr_num] = length - 1;

			claujson::LoadData::parse(*ut, string_buf, tape, length, key, start, thr_num); // 0 : use all thread. - notyet..

			int c = clock();
			std::cout << c - b << "ms\n";
		}
		int c = clock();
		std::cout << c - _ << "ms\n";

		// claujson::LoadData::_save(std::cout, &ut);

		return 0;
	}

	inline int Parse_One(const std::string& str, Data& data) {
		{
			simdjson::dom::parser test;

			auto x = test.parse(str);

			if (x.error() != simdjson::error_code::SUCCESS) {
				std::cout << x.error() << "\n";

				return -1;
			}

			const auto& tape = test.raw_tape();
			const auto& string_buf = test.raw_string_buf();

			data = Convert(&tape[1], string_buf);
		}
		return 0;
	}
}
