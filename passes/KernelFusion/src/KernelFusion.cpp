//
// Created by Aman LaChapelle on 4/6/18.
//
// Hobbit
// Copyright (c) 2018 Aman LaChapelle
// Full license at Hobbit/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#include <polly/DependenceInfo.h>
#include <polly/ScopInfo.h>
#include <polly/ScopPass.h>
#include <polly/Support/ISLOStream.h>

#define DEBUG_TYPE "hobbit-kernel-fusion"

using namespace polly;
using namespace llvm;

namespace {
/// Print a schedule to @p OS.
///
/// Prints the schedule for each statements on a new line.
void printSchedule(raw_ostream &OS, const isl::union_map &Schedule,
                   int indent) {
  Schedule.foreach_map([&OS, indent](isl::map Map) -> isl::stat {
    OS.indent(indent) << Map << "\n";
    return isl::stat::ok;
  });
}

class KernelFusion : public ScopPass {
private:
  KernelFusion(const KernelFusion &) = delete;
  const KernelFusion &operator=(const KernelFusion &) = delete;

  std::shared_ptr<isl_ctx> IslCtx;
  isl::union_map OldSchedule;

public:
  static char ID;
  explicit KernelFusion() : ScopPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredTransitive<ScopInfoRegionPass>();
    AU.addRequiredTransitive<DependenceInfo>();
    AU.setPreservesAll();
  }

  bool runOnScop(Scop &S) override {
    // Keep a reference to isl_ctx to ensure that it is not freed before we
    // free OldSchedule.
    IslCtx = S.getSharedIslCtx();

    // dbgs()
    //      DEBUG(outs() << "Going to flatten old schedule:\n");
    OldSchedule = S.getSchedule();
    printSchedule(outs(), OldSchedule, 2);
    //      DEBUG(printSchedule(outs(), OldSchedule, 2));

    //      auto Domains = S.getDomains();
    //      auto RestrictedOldSchedule =
    //      OldSchedule.intersect_domain(Domains); DEBUG(dbgs() << "Old
    //      schedule with domains:\n"); DEBUG(printSchedule(dbgs(),
    //      RestrictedOldSchedule, 2));
    //
    //      auto NewSchedule = flattenSchedule(RestrictedOldSchedule);
    //
    //      DEBUG(dbgs() << "Flattened new schedule:\n");
    //      DEBUG(printSchedule(dbgs(), NewSchedule, 2));
    //
    //      NewSchedule = NewSchedule.gist_domain(Domains);
    //      DEBUG(dbgs() << "Gisted, flattened new schedule:\n");
    //      DEBUG(printSchedule(dbgs(), NewSchedule, 2));
    //
    //      S.setSchedule(NewSchedule);
    return false;
  }

  void printScop(raw_ostream &OS, Scop &S) const override {
    //      OS << "Schedule before flattening {\n";
    //      printSchedule(OS, OldSchedule, 4);
    //      OS << "}\n\n";
    //
    //      OS << "Schedule after flattening {\n";
    //      printSchedule(OS, S.getSchedule(), 4);
    //      OS << "}\n";
  }

  void releaseMemory() override {
    OldSchedule = nullptr;
    IslCtx.reset();
  }
};
} // namespace

char KernelFusion::ID = 0;
static RegisterPass<KernelFusion> X("hobbit-kernel-fusion",
                                    "Hobbit - Kernel Fusion");