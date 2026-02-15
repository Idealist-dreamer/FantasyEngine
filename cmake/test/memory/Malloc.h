#pragma once

using Alloc = Allocator<AllocType::MiMalloc>;

static bool IsAligned(void* p, size_t alignment) {
  return reinterpret_cast<uintptr_t>(p) % alignment == 0;
}

static bool IsAlignedAt(void* p, size_t alignment, size_t offset) {
  return (reinterpret_cast<uintptr_t>(p) + offset) % alignment == 0;
}

TEST(TestMiMalloc, RawAllocators) {
  const size_t size = 1024;

  // malloc
  void* p1 = Alloc::malloc(size);
  ASSERT_NE(p1, nullptr);
  memset(p1, 0xAA, size);
  Alloc::free(p1);

  // zalloc
  uint8_t* p2 = static_cast<uint8_t*>(Alloc::zalloc(size));
  ASSERT_NE(p2, nullptr);
  for (size_t i = 0; i < size; ++i)
    EXPECT_EQ(p2[i], 0);
  Alloc::free(p2);

  // calloc
  const size_t count = 16;
  const size_t elemSize = 64;
  uint8_t* p3 = static_cast<uint8_t*>(Alloc::calloc(count, elemSize));
  ASSERT_NE(p3, nullptr);
  for (size_t i = 0; i < count * elemSize; ++i)
    EXPECT_EQ(p3[i], 0);
  Alloc::free(p3);
}

TEST(TestMiMalloc, ReallocAndExpand) {
  size_t size = 128;
  uint8_t* p = static_cast<uint8_t*>(Alloc::malloc(size));
  memset(p, 0xBB, size);

  // realloc
  size = 256;
  p = static_cast<uint8_t*>(Alloc::realloc(p, size));
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p[0], 0xBB);

  // reallocN
  p = static_cast<uint8_t*>(Alloc::reallocN(p, 2, size));
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p[0], 0xBB);

  // expand
  bool ok = Alloc::expand(p, 1024);
  if (ok) {
    p[1023] = 0xCC;
    EXPECT_EQ(p[1023], 0xCC);
  }

  Alloc::free(p);
}

TEST(TestMiMalloc, AlignedAllocators) {
  const size_t size = 1024;
  const size_t align = 64;

  // mallocAligned
  void* p1 = Alloc::mallocAligned(size, align);
  EXPECT_TRUE(IsAligned(p1, align));
  Alloc::free(p1);

  // zallocAligned
  uint8_t* p2 = static_cast<uint8_t*>(Alloc::zallocAligned(size, align));
  EXPECT_TRUE(IsAligned(p2, align));
  EXPECT_EQ(p2[0], 0);
  Alloc::free(p2);

  // callocAligned
  uint8_t* p3 = static_cast<uint8_t*>(Alloc::callocAligned(4, 256, align));
  EXPECT_TRUE(IsAligned(p3, align));
  EXPECT_EQ(p3[1023], 0);
  Alloc::free(p3);

  // reallocAligned
  void* p4 = Alloc::mallocAligned(size, align);
  p4 = Alloc::reallocAligned(p4, size * 2, align * 2);
  EXPECT_TRUE(IsAligned(p4, align * 2));
  Alloc::free(p4);
}

TEST(TestMiMalloc, AlignedAtAllocators) {
  const size_t size = 1024;
  const size_t align = 64;
  const size_t offset = 16;

  // mallocAlignedAt
  void* p1 = Alloc::mallocAlignedAt(size, align, offset);
  EXPECT_TRUE(IsAlignedAt(p1, align, offset));
  Alloc::free(p1);

  // zallocAlignedAt
  uint8_t* p2 = static_cast<uint8_t*>(Alloc::zallocAlignedAt(size, align, offset));
  EXPECT_TRUE(IsAlignedAt(p2, align, offset));
  EXPECT_EQ(p2[0], 0);
  Alloc::free(p2);

  // callocAlignedAt
  uint8_t* p3 = static_cast<uint8_t*>(Alloc::callocAlignedAt(2, 512, align, offset));
  EXPECT_TRUE(IsAlignedAt(p3, align, offset));
  EXPECT_EQ(p3[0], 0);
  Alloc::free(p3);

  // reallocAlignedAt
  void* p4 = Alloc::mallocAlignedAt(size, align, offset);
  p4 = Alloc::reallocAlignedAt(p4, size * 2, align, offset);
  EXPECT_TRUE(IsAlignedAt(p4, align, offset));
  Alloc::free(p4);
}

TEST(TestMiMalloc, SafetyAndEdgeCases) {
  // Null free
  EXPECT_NO_THROW(Alloc::free(nullptr));

  // Zero size
  void* p1 = Alloc::malloc(0);
  Alloc::free(p1);

  // Large alignment
  void* p2 = Alloc::mallocAligned(1024, 4096);
  EXPECT_TRUE(IsAligned(p2, 4096));
  Alloc::free(p2);
}