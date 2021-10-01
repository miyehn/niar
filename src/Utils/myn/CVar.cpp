#include "CVar.h"
#include "Log.h"

namespace myn
{
	std::vector<CVarBase*> cvars_list(CVarBase* new_cvar) {
		static std::vector<CVarBase*> ConsoleVariables = std::vector<CVarBase*>();
		if (new_cvar) ConsoleVariables.push_back(new_cvar);
		return ConsoleVariables;
	}

	CVarBase* find_cvar(std::string name) {
		auto ConsoleVariables = cvars_list();
		for (int i=0; i<ConsoleVariables.size(); i++) {
			if (lower(ConsoleVariables[i]->name) == lower(name)) {
				return ConsoleVariables[i];
			}
		}
		return nullptr;
	}

	void list_cvars() {
		auto ConsoleVariables = cvars_list();

		LOGR("There are %lu console variables:", ConsoleVariables.size());
		for (int i=0; i<ConsoleVariables.size(); i++) {
			if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(ConsoleVariables[i])) {
				LOGR("\t%s (int)\t%d", cvar->get_name().c_str(), cvar->get());

			} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(ConsoleVariables[i])) {
				LOGR("\t%s (float)\t%f", cvar->get_name().c_str(), cvar->get());

			}
		}
	}

	void log_cvar(std::string name){
		if (auto found = find_cvar(name)) {
			if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(found)) {
				LOGR("int, %s, %d", cvar->get_name().c_str(), cvar->get());
				return;

			} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(found)) {
				LOGR("float, %s, %f", cvar->get_name().c_str(), cvar->get());
				return;
			}
		}
		LOGR("cvar not found.");
	}

	void set_cvar(std::string name, std::string val) {
		if (auto found = find_cvar(name)) {
			if (CVar<int>* cvar = dynamic_cast<CVar<int>*>(found)) {
				try {
					int n = std::stoi(val);
					cvar->set(n);
				} catch (std::invalid_argument const &e) {
					LOGR("invalid int argument");
				}

			} else if (CVar<float>* cvar = dynamic_cast<CVar<float>*>(found)) {
				try {
					float n = std::stof(val);
					cvar->set(n);
				} catch (std::invalid_argument const &e) {
					LOGR("invalid float argument");
				}

			}
		} else {
			LOGR("cvar not found.");
		}
	}
}// namespace myn
