#pragma once

// Reuse MockObject and AlignedObject from previous context
TEST(TestMiMalloc, ArrayTypeAllocators) {
  const size_t count = 10;

  // 1. Test mallocTypeArray
  int* arr1 = Alloc::mallocTypeArray<int>(count);
  ASSERT_NE(arr1, nullptr);
  for (size_t i = 0; i < count; ++i)
    arr1[i] = static_cast<int>(i);
  EXPECT_EQ(arr1[5], 5);
  Alloc::free(arr1);

  // 2. Test zallocTypeArray (Verify zero initialization)
  int* arr2 = Alloc::zallocTypeArray<int>(count);
  ASSERT_NE(arr2, nullptr);
  for (size_t i = 0; i < count; ++i)
    EXPECT_EQ(arr2[i], 0);
  Alloc::free(arr2);
}

TEST(TestMiMalloc, CreateDestroyArray) {
  MockObject::constructed_count = 0;
  const size_t count = 5;

  // 1. Test createArray with arguments
  MockObject* arr = Alloc::createArray<MockObject>(count);
  ASSERT_NE(arr, nullptr);
  EXPECT_EQ(MockObject::constructed_count, count);

  // for (size_t i = 0; i < count; ++i) {
  //   EXPECT_EQ(arr[i].a_, 100);
  // }

  // 2. Test destroyArray (Verify destructor calls)
  Alloc::destroyArray(arr, count);
  EXPECT_EQ(MockObject::constructed_count, 0);
}

TEST(TestMiMalloc, AlignedArrayIntegrity) {
  const size_t count = 4;
  // AlignedObject has alignas(64)
  AlignedObject* arr = Alloc::createArray<AlignedObject>(count);
  ASSERT_NE(arr, nullptr);

  // Verify alignment of every single element in the array
  for (size_t i = 0; i < count; ++i) {
    EXPECT_TRUE(IsAligned(&arr[i], 64)) << "Element " << i << " misaligned!";
  }

  Alloc::destroyArray(arr, count);
}

TEST(TestMiMalloc, ArraySafety) {
  // Test with count = 0
  int* p0 = Alloc::mallocTypeArray<int>(0);
  // mimalloc usually returns a unique pointer for 0 size, or nullptr
  Alloc::free(p0);

  // Test destroyArray with nullptr
  EXPECT_NO_THROW(Alloc::destroyArray<int>(nullptr, 10));
}
