#pragma once
#include <vector>
#include <string>
#include <mutex>
#include "Misc.h"

namespace myn
{
	class CVarBase;

	std::vector<CVarBase*> cvars_list(CVarBase* new_cvar = nullptr);

	void list_cvars();

	void log_cvar(std::string name);

	void set_cvar(std::string name, std::string val);

	class CVarBase {
	public:
		void register_self() { cvars_list(this); }
		std::string name = "";
	private:
		virtual void __placeholder() {} // just so it has a virtual method
	};

	template<typename T>
	class CVar : public CVarBase {
	public:

		// default constructor
		CVar() { register_self(); }
		// constructor with initialization
		CVar(std::string _name, T _value) : value(_value) {
			name = _name;
			register_self();
		}

		// getter
		T get() const { return value; }
		std::string get_name() const { return name; }

		// setter
		void set(T _value) {
			std::lock_guard<std::mutex> lock(m);
			value = _value;
		}

	private:
		T value;
		std::mutex m;
	};

} // namecpace myn