#include "runtime_state.hpp"

namespace runtime {

State& state() {
    static State instance;
    return instance;
}

}
