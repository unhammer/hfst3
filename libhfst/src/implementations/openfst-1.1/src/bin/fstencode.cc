// fstencode.cc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: riley@google.com (Michael Riley)
//
// \file
//  Encode transducer labels and/or weights.
//

#include "./encode-main.h"

namespace fst {

// Register templated main for common arcs types.
REGISTER_FST_MAIN(EncodeMain, StdArc);
REGISTER_FST_MAIN(EncodeMain, LogArc);

}  // namespace fst


int main(int argc, char **argv) {
  string usage = "Encodes transducer labels and/or weights.\n";
  usage += "  Usage: ";
  usage += argv[0];
  usage += " in.fst codex [out.fst]\n";
  usage += "  Flags: decode, encode_labels, encode_weights\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  // Invokes EncodeMain<Arc> where arc type is determined from argv[1].
  return CALL_FST_MAIN(EncodeMain, argc, argv);
}
