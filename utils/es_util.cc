#include "es_util.h"

using namespace std;
namespace es {

std::unordered_map<int, std::function<void()>> Signal::handlers;
} // namespace es
