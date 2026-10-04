//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//


#include "llvm/Transforms/Utils/HelloWorld.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {


// Dead Code Elimination


static bool runDCE(Function &F) {
  bool Changed = false;

  SmallVector<Instruction *, 32> ToDelete;

  for (auto &BB : F) {
    for (auto &I : BB) {

      // Keep instructions which have users.
      if (!I.use_empty())
        continue;

      // Keep instructions with side effects.
      if (I.mayHaveSideEffects())
        continue;

      // Keep terminators.
      if (I.isTerminator())
        continue;

      // Keep EH landing pads.
      if (isa<LandingPadInst>(&I))
        continue;

      ToDelete.push_back(&I);
    }
  }

  for (Instruction *I : ToDelete) {
    errs() << "[DCE] Removing: " << *I << "\n";
    I->eraseFromParent();
    Changed = true;
  }

  return Changed;
}


// Common Subexpression Elimination


static bool runCSE(Function &F, FunctionAnalysisManager &AM) {
  bool Changed = false;

  DominatorTree &DT =
      AM.getResult<DominatorTreeAnalysis>(F);

  SmallVector<Instruction *, 32> ToDelete;

  // Keep instructions that we have already encountered.
  SmallVector<Instruction *, 32> Seen;

  for (auto &BB : F) {

    // Local expressions for this basic block.
    SmallVector<Instruction *, 32> LocalSeen;

    for (auto &I : BB) {

      if (I.isTerminator())
        continue;

      if (isa<LandingPadInst>(&I))
        continue;

      Instruction *Identical = nullptr;

    
      // Local CSE
    

      for (Instruction *Previous : LocalSeen) {
        if (I.isIdenticalTo(Previous)) {
          Identical = Previous;
          break;
        }
      }

      if (Identical) {
        errs() << "[CSE] Local common subexpression: "
               << I << "\n";

        I.replaceAllUsesWith(Identical);
        ToDelete.push_back(&I);

        Changed = true;
        continue;
      }

    
      // Global CSE
    

      for (Instruction *Previous : Seen) {

        if (!I.isIdenticalTo(Previous))
          continue;

        bool CanReplace = true;

        // The previous instruction must dominate
        // every use of the current instruction.
        for (Use &U : I.uses()) {
          if (!DT.dominates(Previous, U)) {
            CanReplace = false;
            break;
          }
        }

        if (CanReplace) {
          Identical = Previous;
          break;
        }
      }

      if (Identical) {
        errs() << "[CSE] Global common subexpression: "
               << I << "\n";

        I.replaceAllUsesWith(Identical);
        ToDelete.push_back(&I);

        Changed = true;
        continue;
      }

      // This instruction has not been seen before.
      LocalSeen.push_back(&I);
      Seen.push_back(&I);
    }
  }

  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed;
}


// Strength Reduction + Constant Folding


static bool runStrengthReductionAndConstantFolding(Function &F) {
  bool Changed = false;

  SmallVector<Instruction *, 32> ToDelete;

  for (auto &BB : F) {
    for (auto &I : BB) {

      if (!I.isBinaryOp())
        continue;

      auto *Op = dyn_cast<BinaryOperator>(&I);

      Value *Left = Op->getOperand(0);
      Value *Right = Op->getOperand(1);


      // Multiplication


      if (Op->getOpcode() == Instruction::Mul) {

        ConstantInt *LC = dyn_cast<ConstantInt>(Left);
        ConstantInt *RC = dyn_cast<ConstantInt>(Right);

        // Constant Folding
        //     5 * 10 -> 50

        if (LC && RC) {

          int64_t LValue = LC->getSExtValue();
          int64_t RValue = RC->getSExtValue();

          Value *Result =
              ConstantInt::get(Op->getType(), LValue * RValue);

          errs() << "[CF] "
                 << LValue << " * " << RValue
                 << " -> " << LValue * RValue << "\n";

          Op->replaceAllUsesWith(Result);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // x * 1 -> x

        if (LC && LC->getSExtValue() == 1) {

          errs() << "[SR] 1 * x -> x\n";

          Op->replaceAllUsesWith(Right);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        if (RC && RC->getSExtValue() == 1) {

          errs() << "[SR] x * 1 -> x\n";

          Op->replaceAllUsesWith(Left);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // x * power_of_two -> x << log2(power_of_two)

        ConstantInt *Power = LC ? LC : RC;
        Value *Variable = LC ? Right : Left;

        if (Power) {

          int64_t Value = Power->getSExtValue();

          if (Value > 0 &&
              (Value & (Value - 1)) == 0) {

            unsigned ShiftAmount = 0;
            int64_t Temp = Value;

            while (Temp != 1) {
              Temp >>= 1;
              ++ShiftAmount;
            }

            IRBuilder<> Builder(Op);

            Value *Shift =
                Builder.CreateShl(
                    Variable,
                    ConstantInt::get(
                        Variable->getType(),
                        ShiftAmount));

            errs() << "[SR] "
                   << Value << " * x -> x << "
                   << ShiftAmount << "\n";

            Op->replaceAllUsesWith(Shift);
            ToDelete.push_back(Op);

            Changed = true;
            continue;
          }
        }
      }


      // Addition


      if (Op->getOpcode() == Instruction::Add) {

        ConstantInt *LC = dyn_cast<ConstantInt>(Left);
        ConstantInt *RC = dyn_cast<ConstantInt>(Right);

        // Constant folding
        if (LC && RC) {

          int64_t LValue = LC->getSExtValue();
          int64_t RValue = RC->getSExtValue();

          Value *Result =
              ConstantInt::get(
                  Op->getType(),
                  LValue + RValue);

          errs() << "[CF] "
                 << LValue << " + " << RValue
                 << " -> " << LValue + RValue << "\n";

          Op->replaceAllUsesWith(Result);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // 0 + x -> x
        if (LC && LC->getSExtValue() == 0) {

          errs() << "[SR] 0 + x -> x\n";

          Op->replaceAllUsesWith(Right);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // x + 0 -> x
        if (RC && RC->getSExtValue() == 0) {

          errs() << "[SR] x + 0 -> x\n";

          Op->replaceAllUsesWith(Left);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }
      }


      // Subtraction


      if (Op->getOpcode() == Instruction::Sub) {

        ConstantInt *LC = dyn_cast<ConstantInt>(Left);
        ConstantInt *RC = dyn_cast<ConstantInt>(Right);

        // Constant folding
        if (LC && RC) {

          int64_t LValue = LC->getSExtValue();
          int64_t RValue = RC->getSExtValue();

          Value *Result =
              ConstantInt::get(
                  Op->getType(),
                  LValue - RValue);

          errs() << "[CF] "
                 << LValue << " - " << RValue
                 << " -> " << LValue - RValue << "\n";

          Op->replaceAllUsesWith(Result);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // x - 0 -> x
        if (RC && RC->getSExtValue() == 0) {

          errs() << "[SR] x - 0 -> x\n";

          Op->replaceAllUsesWith(Left);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }
      }


      // Division


      if (Op->getOpcode() == Instruction::SDiv ||
          Op->getOpcode() == Instruction::UDiv) {

        ConstantInt *LC = dyn_cast<ConstantInt>(Left);
        ConstantInt *RC = dyn_cast<ConstantInt>(Right);

        // Constant folding
        if (LC && RC) {

          int64_t LValue = LC->getSExtValue();
          int64_t RValue = RC->getSExtValue();

          // Avoid division by zero.
          if (RValue != 0) {

            Value *Result =
                ConstantInt::get(
                    Op->getType(),
                    LValue / RValue);

            errs() << "[CF] "
                   << LValue << " / " << RValue
                   << " -> " << LValue / RValue << "\n";

            Op->replaceAllUsesWith(Result);
            ToDelete.push_back(Op);

            Changed = true;
            continue;
          }
        }

        // x / 1 -> x
        if (RC && RC->getSExtValue() == 1) {

          errs() << "[SR] x / 1 -> x\n";

          Op->replaceAllUsesWith(Left);
          ToDelete.push_back(Op);

          Changed = true;
          continue;
        }

        // x / power_of_two -> x >> log2(power_of_two)
        if (RC) {

          int64_t Value = RC->getSExtValue();

          if (Value > 0 &&
              (Value & (Value - 1)) == 0) {

            unsigned ShiftAmount = 0;
            int64_t Temp = Value;

            while (Temp != 1) {
              Temp >>= 1;
              ++ShiftAmount;
            }

            IRBuilder<> Builder(Op);

            Value *Shift =
                Builder.CreateAShr(
                    Left,
                    ConstantInt::get(
                        Left->getType(),
                        ShiftAmount));

            errs() << "[SR] x / "
                   << Value
                   << " -> x >> "
                   << ShiftAmount << "\n";

            Op->replaceAllUsesWith(Shift);
            ToDelete.push_back(Op);

            Changed = true;
            continue;
          }
        }
      }
    }
  }

  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed;
}

} // anonymous namespace


// HelloWorldPass


PreservedAnalyses
HelloWorldPass::run(Function &F,
                    FunctionAnalysisManager &AM) {

  errs() << "\n";
  errs() << "========================================\n";
  errs() << "Running custom optimization passes on: "
         << F.getName() << "\n";
  errs() << "========================================\n";

  bool Changed = false;


  // 1. Strength Reduction + Constant Folding


  Changed |= runStrengthReductionAndConstantFolding(F);


  // 2. Common Subexpression Elimination


  Changed |= runCSE(F, AM);


  // 3. Dead Code Elimination


  Changed |= runDCE(F);


  // Repeat DCE once more because CSE/SR/CF can create dead code.


  if (Changed)
    Changed |= runDCE(F);

  errs() << "Finished custom optimization passes on: "
         << F.getName() << "\n\n";

  if (Changed)
    return PreservedAnalyses::none();

  return PreservedAnalyses::all();
}
