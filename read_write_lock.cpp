#include <iostream>
#include <thread>

class RWLock {

public:
  RWLock() : lock_count(0), waiter(0) {}
  void RLock();
  void URLock();
  void WLock();
  void UWLock();

private:
  std::mutex mut_;
  std::condition_variable cond_;
  int lock_count;   // -1,W lock; >0, R lock 
  int waiter;
};

void RWLock::RLock() {
  std::unique_lock<std::mutex> lock(mut_);
  waiter++;
  if (lock_count == -1) {
    cond_.wait(lock, [this]() { return lock_count >= 0; });
  }
  lock_count++;
  waiter--;
}

void RWLock::URLock() {
  std::unique_lock<std::mutex> lock(mut_);
  if (lock_count > 0) {
    lock_count--;
    if (waiter > 0 && lock_count == 0) {
      cond_.notify_one();
    }
  } else {
    exit(-1);
  }
}

void RWLock::WLock() {
  std::unique_lock<std::mutex> lock(mut_);
  waiter++;
  if (lock_count != 0) {
    cond_.wait(lock, [this]() { return lock_count == 0; });
  }
  lock_count = -1;
  waiter--;
}

void RWLock::UWLock() {
  std::unique_lock<std::mutex> lock(mut_);
  if (lock_count == -1) {
    lock_count = 0;
    if (waiter > 0) {
      cond_.notify_all();
    }
  } else {
    exit(-1);
  }
}

int main() {
  RWLock lock;
  auto read = [&lock]() {
    using namespace std::chrono_literals;
    lock.RLock();
    std::this_thread::sleep_for(2000ms);
    std::cout << "read finish\n";
    lock.URLock();
  };
  auto write = [&lock]() {
    using namespace std::chrono_literals;
    lock.WLock();
    std::this_thread::sleep_for(2000ms);
    std::cout << "write finish\n";
    lock.UWLock();
  };
  using namespace std::chrono_literals;
  std::thread th1(write);
  std::this_thread::sleep_for(200ms);
  std::thread th2(read);
  std::thread th3(read);
  std::this_thread::sleep_for(4000ms);
  std::thread th4(write);
  std::thread th5(write);
  std::this_thread::sleep_for(200ms);
  std::thread th6(read);
  std::thread th7(read);

  th1.join();
  th2.join();
  th3.join();
  th4.join();
  th5.join();
  th6.join();
  th7.join();

  return 0;
}