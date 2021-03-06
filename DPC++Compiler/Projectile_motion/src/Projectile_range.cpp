//=============================================================
// Copyright � 2019 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <CL/sycl.hpp>
#include <vector>
#include "Projectile.hpp"

using namespace cl::sycl;
using namespace std;

static const int num_elements = 100;
const double kPIValue = 3.1415;
const double kGValue = 9.81;

// Function to calculate the range, maximum height and total flight time of a
// projectile
inline void CalculateRange(Projectile& obj, Projectile& pObj) {  
  
  float proj_angle = obj.getangle();
  float proj_vel = obj.getvelocity();
  // for trignometric functions use cl::sycl::sin/cos
  float sin_value = cl::sycl::sin(proj_angle * kPIValue / 180);
  float cos_value = cl::sycl::cos(proj_angle * kPIValue / 180);
  float total_time = cl::sycl::fabs((2 * proj_vel * sin_value)) / kGValue;
  float max_range = cl::sycl::fabs(proj_vel * total_time * cos_value);
  float max_height = (proj_vel * proj_vel * sin_value * sin_value) / 2 *
                     kGValue;  // h = v^2 * sin^2theta/2g

  pObj.setRangeandTime(max_range, total_time, proj_angle, proj_vel, max_height);
}

// Compare the results of the two output vectors from parallel and scalar. They
// should be equal
int Compare(std::vector<Projectile>& v1, std::vector<Projectile>& v2) {
  int ret_code = 1;
  for (int i = 0; i < num_elements; i++) {
    if (v1[i] != v2[i]) {
      ret_code = -1;
      break;
    }
  }
  return ret_code;
}

// in_vect and out_vect are the vectors with N Projectile numbers and are inputs to the
// parallel function
void DpcppParallel(queue& q, std::vector<Projectile>& in_vect,
                    std::vector<Projectile>& out_vect) {
  buffer<Projectile, 1> bufin_vect(in_vect.data(), range<1>(num_elements));
  buffer<Projectile, 1> bufout_vect(out_vect.data(), range<1>(num_elements));

  std::cout << "Target Device: "
            << q.get_device().get_info<info::device::name>() << "\n";

  // Submit Command group function object to the queue
  q.submit([&](handler& h) {
    // Input accessors set as read_write mode
    auto V1 = bufin_vect.get_access<access::mode::read_write>(h);

    // Output accessor set to write mode.
    auto V2 = bufout_vect.get_access<access::mode::write>(h);

    h.parallel_for(range<1>(num_elements), [=](id<1> i) {
      // Call the Inline function calculate_range
      CalculateRange(V1[i], V2[i]);
    });
  });
  q.wait_and_throw();
}
// scalar function to calculate the range
void DpcppScalar(std::vector<Projectile>& v1, std::vector<Projectile>& v2) {
  for (int i = 0; i < num_elements; i++) {
    CalculateRange(v1[i], v2[i]);
  }
}

int main() {
  srand(time(NULL));
  float init_angle = 0;
  float init_vel = 0;
  vector<Projectile> input_vect1, out_parallel_vect2, out_scalar_vect3;
  // Initialize the Input and Output vectors
  for (int i = 0; i < num_elements; i++) {
    init_angle = rand() % 90 + 10;
    init_vel = rand() % 400 + 10;
    input_vect1.push_back(Projectile(init_angle, init_vel, 1.0, 1.0, 1.0));
    out_parallel_vect2.push_back(Projectile());
    out_scalar_vect3.push_back(Projectile());
  }

  // this exception handler with catch async exceptions
  auto exception_handler = [&](cl::sycl::exception_list eList) {
    for (std::exception_ptr const& e : eList) {
      try {
        std::rethrow_exception(e);
      } catch (cl::sycl::exception const& e) {
        std::cout << "Failure" << std::endl;
        std::terminate();
      }
    }
  };
  try {
    // queue constructor passed exception handler
    queue q(default_selector{}, exception_handler);
    // Call the DpcppParallel with the required inputs and outputs
    DpcppParallel(q, input_vect1, out_parallel_vect2);
  } catch (...) {
    // some other exception detected
    std::cout << "Failure" << std::endl;
    std::terminate();
  }

  // call the DpcppScalar with input_vect1 as input and out_scalar_vect3 as
  // output
  DpcppScalar(input_vect1, out_scalar_vect3);
  for (int i = 0; i < num_elements; i++) {
    // Displaying the Scalar computation results. Uncomment to viee the results
    // cout<<"Scalar "<<out_scalar_vect3[i];
  }
  for (int i = 0; i < num_elements; i++) {
    // Displaying the Parallel computation results.
    cout << "Parallel " << out_parallel_vect2[i];
  }
  // Compare the vectors of both the outputs of parallal and scalar functions
  int ret_code = Compare(out_parallel_vect2, out_scalar_vect3);
  if (ret_code == 1) {
    cout
        << "********************************************Success..The Results "
           "are matched********************************************************"
        << "\n";
  } else
    cout << "*********************************************Failed***************"
            "*****************************************"
         << "\n";
  return 0;
}
