// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Main entry point for all unit tests.

#include "rlz_test_helpers.h"

#include <map>
#include <vector>

#include "base/strings/string16.h"
#include "rlz/lib/rlz_lib.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include <shlwapi.h>
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#elif defined(OS_POSIX)
#include "base/files/file_path.h"
#include "rlz/lib/rlz_value_store.h"
#endif

#if defined(OS_WIN)

namespace {

// Path to recursively copy into the replacemment hives.  These are needed
// to make sure certain win32 APIs continue to run correctly once the real
// hives are replaced.
const wchar_t kHKLMAccessProviders[] =
    L"System\\CurrentControlSet\\Control\\Lsa\\AccessProviders";

struct RegistryValue {
  base::string16 name;
  DWORD type;
  std::vector<uint8> data;
};

struct RegistryKeyData {
  std::vector<RegistryValue> values;
  std::map<base::string16, RegistryKeyData> keys;
};

void ReadRegistryTree(const base::win::RegKey& src, RegistryKeyData* data) {
  // First read values.
  {
    base::win::RegistryValueIterator i(src.Handle(), L"");
    data->values.clear();
    data->values.reserve(i.ValueCount());
    for (; i.Valid(); ++i) {
      RegistryValue& value = *data->values.insert(data->values.end(),
                                                  RegistryValue());
      const uint8* data = reinterpret_cast<const uint8*>(i.Value());
      value.name.assign(i.Name());
      value.type = i.Type();
      value.data.assign(data, data + i.ValueSize());
    }
  }

  // Next read subkeys recursively.
  for (base::win::RegistryKeyIterator i(src.Handle(), L"");
       i.Valid(); ++i) {
    ReadRegistryTree(base::win::RegKey(src.Handle(), i.Name(), KEY_READ),
                     &data->keys[base::string16(i.Name())]);
  }
}

void WriteRegistryTree(const RegistryKeyData& data, base::win::RegKey* dest) {
  // First write values.
  for (size_t i = 0; i < data.values.size(); ++i) {
    const RegistryValue& value = data.values[i];
    dest->WriteValue(value.name.c_str(),
                     value.data.size() ? &value.data[0] : NULL,
                     static_cast<DWORD>(value.data.size()),
                     value.type);
  }

  // Next write values recursively.
  for (std::map<base::string16, RegistryKeyData>::const_iterator iter =
           data.keys.begin();
       iter != data.keys.end(); ++iter) {
    WriteRegistryTree(iter->second,
                      &base::win::RegKey(dest->Handle(), iter->first.c_str(),
                                         KEY_ALL_ACCESS));
  }
}

// Initialize temporary HKLM/HKCU registry hives used for testing.
// Testing RLZ requires reading and writing to the Windows registry.  To keep
// the tests isolated from the machine's state, as well as to prevent the tests
// from causing side effects in the registry, HKCU and HKLM are overridden for
// the duration of the tests. RLZ tests don't expect the HKCU and KHLM hives to
// be empty though, and this function initializes the minimum value needed so
// that the test will run successfully.
void InitializeRegistryOverridesForTesting(
    registry_util::RegistryOverrideManager* override_manager) {
  // For the moment, the HKCU hive requires no initialization.
  const bool do_copy = (base::win::GetVersion() >= base::win::VERSION_WIN7);
  RegistryKeyData data;

  if (do_copy) {
    // Copy the following HKLM subtrees to the temporary location so that the
    // win32 APIs used by the tests continue to work:
    //
    //    HKLM\System\CurrentControlSet\Control\Lsa\AccessProviders
    //
    // This seems to be required since Win7.
    ReadRegistryTree(base::win::RegKey(HKEY_LOCAL_MACHINE,
                                       kHKLMAccessProviders,
                                       KEY_READ), &data);
  }

  override_manager->OverrideRegistry(HKEY_LOCAL_MACHINE, L"rlz_temp_hklm");
  override_manager->OverrideRegistry(HKEY_CURRENT_USER, L"rlz_temp_hkcu");

  if (do_copy) {
    WriteRegistryTree(data, &base::win::RegKey(HKEY_LOCAL_MACHINE,
                                               kHKLMAccessProviders,
                                               KEY_ALL_ACCESS));
  }
}

}  // namespace

#endif  // defined(OS_WIN)

void RlzLibTestNoMachineState::SetUp() {
#if defined(OS_WIN)
  InitializeRegistryOverridesForTesting(&override_manager_);
#elif defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool pool;
#endif  // defined(OS_WIN)
#if defined(OS_POSIX)
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  rlz_lib::testing::SetRlzStoreDirectory(temp_dir_.path());
#endif  // defined(OS_POSIX)
}

void RlzLibTestNoMachineState::TearDown() {
#if defined(OS_POSIX)
  rlz_lib::testing::SetRlzStoreDirectory(base::FilePath());
#endif  // defined(OS_POSIX)
}

void RlzLibTestBase::SetUp() {
  RlzLibTestNoMachineState::SetUp();
#if defined(OS_WIN)
  rlz_lib::CreateMachineState();
#endif  // defined(OS_WIN)
}
