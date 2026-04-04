#include "Mutex.h"
#include <vector>
#include "Support/basics.h"  // assert()
#include <algorithm>  // std::find()

namespace V3DLib {
namespace Mutex {
namespace {

/**
 * @brief Find index of given element in vector
 *
 * This function does not take multiple instances of element into account.
 *
 * Source: https://www.delftstack.com/howto/cpp/find-in-vector-in-cpp/
 *
 * @return index of element if found, -1 otherwise
 */
int find_in_vector(std::vector<int> const &vec, int element) {
  auto it = std::find(vec.begin(), vec.end(), element);

  if (it != vec.end()) {
    int index = (int) std::distance(vec.begin(), it);
    //warn << "Element " << element << " found at index: " << index;
    return index;
  } else {
    //warn << "Element not found";
    return -1;
  }
}


/**
 * @brief List of QPU's which (tried to) acquire the mutex.
 *
 * If empty, no mutex acquired.
 * Otherwise, first item has the mutex. The rest in the list is blocked by the mutex.
 * Any other QPU's (not in the list) are not blocked.
 */
std::vector<int> s_acquire_list;

} // anon namespace


/**
 * @return true if mutex acquired, false otherwise
 */
bool acquire(int qpu_number) {
  int index = find_in_vector(s_acquire_list, qpu_number);

  if (index != -1) {
    // This is called twice for an OR-operation reading SPECIAL_MUTEX_ACQUIRE,
    // because the special register is both in add srcA and add srcB.
    // So we need to allow multiple calls to this special register.

    // Already present, don't add again
    return (index == 0);
  } else {
    s_acquire_list.push_back(qpu_number);
    return true;
  }
}


/**
 * Only the QPU holding the mutex member can release.
 *
 * @return true if mutex released, false otherwise
 */
bool release(int qpu_number) {
  int index = find_in_vector(s_acquire_list, qpu_number);

  // Not expecting mutex to be released when not acquired.
  // Technically, it is possible. Deal with it if it happens.
  if (index != 0) {
    assert(false);
    return false;
  }

  assertq(index == 0, "mutex_release() called but was not acquired");

  s_acquire_list.erase(s_acquire_list.begin());
  return true; 
}


/**
 * @brief Check if qpu_number is blocked by the mutex
 *
 * QPU's which did not call acquired() are free to run.
 *
 * @return true if QPU blocked, false otherwise
 */
bool blocks(int qpu_number) {
  if (s_acquire_list.empty()) return false;  // Mutex not held

  // Only first item is not blocked.
  // All other qpu_numbers in the list, should be blocked
  int index = find_in_vector(s_acquire_list, qpu_number);

  if (index == -1) return false;
  return (index != 0);
}

} // namespace Mutex
} // namespace V3DLib


