#include "TrafficLight.h"
#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */

template <typename T> T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and
  // _condition.wait() to wait for and receive new messages and pull them from
  // the queue using move semantics. The received object should then be returned
  // by the receive function.
  // perform queue modification under the lock
  std::unique_lock<std::mutex> uLock(_mutex);
  _cond.wait(uLock, [this] {
    return !_queue.empty();
  }); // pass unique lock to condition variable

  // remove last vector element from queue
  T msg = std::move(_queue.back());
  _queue.pop_back();

  return msg; // will not be copied due to return value optimization (RVO) in
              // C++
}

template <typename T> void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new
  // message to the queue and afterwards send a notification.
  // simulate some work
  // perform vector modification under the lock
  std::lock_guard<std::mutex> uLock(_mutex);

  // add vector to queue
  std::cout << "Message added to the queue\n";
  _queue.push_back(std::move(msg));
  _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() { _currentPhase = TrafficLightPhase::red; }

void TrafficLight::waitForGreen() {
  // FP.5b : add the implementation of the method waitForGreen, in which an
  // infinite while-loop runs and repeatedly calls the receive function on the
  // message queue. Once it receives TrafficLightPhase::green, the method
  while (true) {
    TrafficLightPhase phase = _msgQueue.receive();
    if (phase == TrafficLightPhase::green)
      return;
  }
}

TrafficLightPhase TrafficLight::getCurrentPhase() { return _currentPhase; }

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started
  // in a thread when the public method „simulate“ is called. To do this, use
  // the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the time
  // between two loop cycles
  std::random_device rd; // Get the Random number from entropy pool

  std::mt19937 prng_eng(
      rd()); // Pass the Random Number as seed to PRNG Mersenne Twister

  std::uniform_real_distribution<> dist(
      4.0, 6.0); // Uniform Distribution between 4 and 6 Seconds
  float cycleTime = dist(prng_eng);

  // init variables for measuring time
  auto start = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> elapsed;

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elapsed = std::chrono::high_resolution_clock::now() - start;

    if (elapsed.count() > cycleTime) {
      // reset start time and get new cycle time
      start = std::chrono::high_resolution_clock::now();
      cycleTime = dist(prng_eng);
      // toggle phase
      _currentPhase = (_currentPhase == TrafficLightPhase::green)
                          ? TrafficLightPhase::red
                          : TrafficLightPhase::green;

      // send update method to message queue using move semantics.
      TrafficLightPhase p = _currentPhase;
      _msgQueue.send(std::move(p));
    }
  }
}
