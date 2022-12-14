#include <map>
#include <vector>
#include <string>
#include <tuple>
#include "skeleton.h"

char SkeletonPass::ID = 0;

// key: address of malloc instruction
// value: size of the array
std::map<Value *, Value *> addressSizeMap;
std::map<Value *, bool> freeMap;

typedef std::map<Value *, std::tuple<Value *, Value *>> mmap;

// 함수 template을 이용한 general한 버전
template <typename K, typename V>
void print_map(std::map<K, V>& m) {
    for (typename std::map<K, V>::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        errs() << itr->first << "\n";
        // << itr->second << "\n";
    }
}

mmap memoryMap;

static void registerSkeletonPass(const PassManagerBuilder &, legacy::PassManagerBase &PM)
{
    // you need to add other pass to use it in the your own pass.
    // For example,

    // You only need to add the above code when necessary.

    // You must write the below code
    PM.add(new SkeletonPass());
}

void SkeletonPass::getAnalysisUsage(AnalysisUsage &AU) const
{
    // you need to add other pass to use it in the your own pass.
    // For example,

    // You only need to add the function when necessary.
}

bool SkeletonPass::runOnModule(Module &M)
{
    // DataLayout: A parsed version of the target data layout string in and methods for querying it
    // EmitGEPOffset 인자
    // DL = &M.getDataLayout();
    DataLayout DL = M.getDataLayout();

    std::vector<Type *> plist;
    plist.push_back(Type::getInt64Ty(M.getContext()));
    plist.push_back(Type::getInt64Ty(M.getContext()));
    
    FunctionType *CompareFunctionType = FunctionType::get(Type::getInt32Ty(M.getContext()), plist, false);
    Function *compareFunc = Function::Create(CompareFunctionType, Function::ExternalLinkage, "compareOffsetNSize", &M);
    

    errs() << "---------------------------------------------------------------------------------";
    for (Function &F : M)
    {
        if (F.hasName())
        {
            errs() << "\nFunction name is... " << F.getName() << "\n";
            // print_map(memoryMap);
        }

        for (BasicBlock &B : F)
        {
            for (Instruction &I : B)
            {
                // debugging output
                errs() << "\n>> this instruction is ..." << I << "\n";

                // Branch Instruction test
                if (BranchInst *BrI = dyn_cast<BranchInst>(&I))
                {
                    errs() << "Branch Instruction !!!\n";
                }

                // 1) Alloca
                if (AllocaInst *AI = dyn_cast<AllocaInst>(&I))
                {
                    // pointer
                    if (AI->getAllocatedType()->isPointerTy())
                    {
                        IRBuilder<> irb(AI);
                        AllocaInst *newAI = irb.CreateAlloca(irb.getInt32Ty(), nullptr);
                        addressSizeMap[dyn_cast<Value>(AI)] = dyn_cast<Value>(newAI);

                        freeMap[dyn_cast<Value>(AI)] = false;

                        errs() << "pointer is allocated\n";
                        errs() << dyn_cast<Value>(AI) << ", " << dyn_cast<Value>(newAI) << "\n";
                        errs() << "!!!!!!! free map value: " << freeMap[dyn_cast<Value>(AI)];
                    }

                    // array
                    if (AI->getAllocatedType()->isArrayTy())
                    {
                        IRBuilder<> irb(AI);
                        AllocaInst *newAI = irb.CreateAlloca(irb.getInt32Ty(), nullptr);

                        ArrayType *arraytype = dyn_cast<ArrayType>(AI->getAllocatedType());
                        Value *arraysize = ConstantInt::get(irb.getInt32Ty(), arraytype->getArrayNumElements());
                        errs() << "arraysize: " << *arraysize << "\n";

                        addressSizeMap[dyn_cast<Value>(AI)] = arraysize;
                    }
                }

                if (CallInst *CI = dyn_cast<CallInst>(&I))
                {
                    errs() << "Call inst! \n";

                    // 2) Call malloc
                    Function *calledF = cast<CallInst>(&I)->getCalledFunction();
                    if (calledF->getName() == "malloc")
                    {
                        errs() << "malloc ! 요놈 잡았따\n";

                        Value *size = CI->getArgOperand(0);
                        errs() << "malloc size: " << *(CI->getArgOperand(0)) << "\n";
                        // errs() << "malloc size: " << *(CI->getOperand(0)) << "\n";

                        Instruction *next = I.getNextNode();
                        errs() << "next: " << *next << "\n";

                        // malloc -> Store
                        if (StoreInst *SI = dyn_cast<StoreInst>(next))
                        {
                            // *ptr: store되는 변수의 alloca instruction 나옴
                            Value *ptr = SI->getPointerOperand();
                            errs() << "*ptr: " << *ptr << "\n";
                            // errs() << "ptr: " << ptr << "\n";

                            // key: Alloca address, value: malloc size
                            addressSizeMap[ptr] = size;
                            // errs() << "size: " << size << "\n";
                            // errs() << "MALLOC size: " << *size << "!!!!!!!!!!!!!!!!\n";

                            IRBuilder<> irb(SI->getNextNode());
                            Value* baseptr = SI->getPointerOperand();
                            ptr = irb.CreateIntCast(ptr, Type::getInt8PtrTy(ptr->getContext()), true);
                            size = irb.CreateIntCast(size, Type::getInt64Ty(size->getContext()), true);
                            Value *ptr2 = irb.CreateAdd(ptr, size);
                            // errs() << *ptr2 << "\n";
                            // ptr2 = irb.CreateIntCast(ptr2, Type::getInt8PtrTy(ptr2->getContext()), true);

                            memoryMap[baseptr] = std::make_tuple(baseptr, ptr2);
                            //print_map(memoryMap);
                        }
                    }

                    // 4) UAF
                    if (calledF->getName() == "free")
                    {
                        errs() << "******** I'm free!!\n";
                        Value *temp = CI->getArgOperand(0);
                        errs() << "******** temp:  " << *temp << "\n";

                        if (LoadInst *LI = dyn_cast<LoadInst>(temp))
                        {
                            
                            // IRBuilder<> irb(CI->getNextNode());
                            // errs() << "next node: " << CI->getNextNode() << "\n";

                            Value *alloca = LI->getPointerOperand();
                            errs() << "******** alloca: " << *alloca << "\n";

                            /* Add map func */
                            if (freeMap.count(alloca))
                            {
                                freeMap[alloca] = true;
                                errs() << "########## isFree???" << freeMap[alloca] << "\n";
                            }

                            // Constant *Null = ConstantPointerNull::get(Type::getInt8PtrTy(M.getContext()));
                            // errs() << "Null: " << *Null << "\n";
 
                            // Value *store = irb.CreateStore(Null, alloca);
                            // errs() << "store: " << *store << "\n";
                        }
                    }
                }

                // 3) GEP
                if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(&I))
                {
                    errs() << "gep instruction! ! ! !\n";

                    IRBuilder<> irb(GEPI);
                    Value *offset = (EmitGEPOffset(&irb, DL, GEPI, true));
                    errs() << "offset: " << *offset << "\n";
                    // *offset:  i64 4*(index)
                    // *offset:  %idxprom4 = sext i32 %9 to i64

                    if (SExtInst *SEI = dyn_cast<SExtInst>(offset))
                    {
                        // errs() << "sext inst prev node: " << *(SEI->getPrevNode()) << "\n";
                        // LoadInst *LI = dyn_cast<LoadInst>(SEI->getPrevNode());
                        // errs() << "Load inst argval: " << *(LI->getOperand(0)) << "\n";
                    }
                    else
                    { // 4로 나눠주는 instruction generation

                        Value *alloca = irb.CreateAlloca(Type::getInt64Ty(M.getContext()), 0, "four");
                        errs() << "alloca: " << *alloca << "\n";

                        Constant *Four = ConstantInt::get(IntegerType::getInt64Ty(M.getContext()), 4);
                        errs() << "Four: " << *Four << "\n";

                        Value *store = irb.CreateStore(Four, alloca);
                        errs() << "store: " << *store << "\n";

                        Value *load = irb.CreateLoad(Type::getInt64Ty(M.getContext()), alloca);
                        errs() << "load: " << *load << "\n";

                        offset = irb.CreateIntCast(offset, Type::getInt64Ty(offset->getContext()), true);

                        offset = irb.CreateUDiv(offset, load);
                        errs() << "offset: " << *offset << "\n";
                    }

                    // GEP instruction -> Store instruction
                    Instruction *next = I.getNextNode();
                    errs() << "next: " << *next << "\n";

                    if (StoreInst *SI = dyn_cast<StoreInst>(next))
                    {
                        errs() << "gep -> store !!! found it!\n";
                        errs() << "GEPI: " << *GEPI << "\n\n";

                        Value *gepPtrOperand = GEPI->getPointerOperand(); // alloca inst
                        errs() << "일단 GEP instructiondml ptr operand: " << *gepPtrOperand << "\n\n";

                        AllocaInst *AI;

                        if (AI = dyn_cast<AllocaInst>(gepPtrOperand))
                        {
                            errs() << "우리가 저장했던 key alloca inst: " << *AI << "\n";
                        }
                        else if (LoadInst *LI = dyn_cast<LoadInst>(gepPtrOperand))
                        {
                            errs() << "gep에 load inst 쓰임! \n";
                            AI = dyn_cast<AllocaInst>(LI->getPointerOperand());
                            errs() << "우리가 저장했던 key alloca inst: " << *AI << "\n";
                        }

                        // map에 있는지 찾기
                        if (addressSizeMap.count(AI))
                        {
                            Value *arraySize = addressSizeMap[AI];
                            Value *offsetSize = offset;

                            errs() << "arraySize: " << *arraySize << "\n";
                            errs() << "offsetSize: " << *offsetSize << "\n\n";
                            // arraySize: i32 3, offsetSize: i64 16
                            // arraySize: i64 6, offsetSize: instruction

                            if (offset->getType()->isIntegerTy())
                            {
                                errs() << "is integer ~~\n";

                                arraySize = irb.CreateIntCast(arraySize, Type::getInt64Ty(arraySize->getContext()), true);
                                offsetSize = irb.CreateIntCast(offset, Type::getInt64Ty(offset->getContext()), true);
                            }
                            else if (dyn_cast<Instruction>(offset))
                            {
                                errs() << "is NOT an integer\n";
                            }

                            errs() << "arraySize: " << *arraySize << "\n";
                            errs() << "offsetSize: " << *offsetSize << "\n\n";

                            // ---------------------------------------------------------------------------------------------------------------
                            IRBuilder<> irb(SI);
                            Value *funcOp1 = offsetSize;
                            Value *funcOp2 = arraySize;

                            std::vector<Value *> paramList;
                            paramList.push_back(funcOp1);
                            paramList.push_back(funcOp2);

                            Value *createdCallInst = irb.CreateCall(compareFunc, paramList);
                        }
                    }
                }

                // 4) Load

                /* 이 부분 말록 잡을 때 걸려서 에러남

                if (LoadInst *LI = dyn_cast<LoadInst>(&I))
                {
                    errs() << "########### Load instruction!!!!!!!\n";
                    AllocaInst *AI = dyn_cast<AllocaInst>(LI->getPointerOperand());
                    // errs() << (AI->getNameOrAsOperand()) << "\n";
                    std::string varName = AI->getNameOrAsOperand();

                    StringRef str = AI->getNameOrAsOperand();

                    // char * var = dyn_cast<char *>(varName);
                    // Value * var2 = dyn_cast<Value>(var);

                    // StringRef varNameinStrRef = AI->getNameOrAsOperand();
                    // Value *varNameInValueTy = dyn_cast<Value *>(varNameinStrRef);
                    // char *str = AI->getNameOrAsOperand();

                    if (freeMap.count(AI) && freeMap[AI] == true)
                    {
                        errs() << "free된 변수입니다. " << freeMap[AI] << "\n";
                        errs() << "******* AI arg: " << varName << "\n";

                        

                        // std::vector<char> chars {'g', 'h', 'T', 'U', 'q', '%', '+', '!', '1', '2', '3'};
                        // std::vector<int8_t> nums = {};
                        // for (auto &number : chars) {
                        //     errs() << "The ASCII value of '" << number << "' is: " << int(number) << "\n";
                        //     nums.push_back(int(number));
                        // }
                        std::vector<Type *> plist2;
                        plist2.push_back(Type::getInt8PtrTy(M.getContext()));

                        FunctionType *exitFuncType = FunctionType::get(Type::getVoidTy(M.getContext()), plist2 , false);
                        Function *exitFunc = Function::Create(exitFuncType, Function::ExternalLinkage, "exitFunc", &M);


                        

                        IRBuilder<> builder(LI);
                        // std::vector<Value *> paramList2;
                        // paramList2.push_back();
                        // llvm::ArrayRef<llvm::Value*> argArrayRef(paramList2);
                        // Value *createdCallInst = irb.CreateCall(exitFunc, argArrayRef);

                        // Function* myPrint = M.getFunction("printf");
                        // std::vector<llvm::Value*> paramArrayRef;
				        // Value *a = ConstantInt::get(Type::getInt8PtrTy(M.getContext()), varName);
				        // paramArrayRef.push_back(a);
                        // CallInst *call_print = CallInst::Create(myPrint, paramArrayRef, "", LI);

                        // Value *callInst = CallInst::Create(exitFunc, ArrayRef<Value *>{ConstantDataArray::get(M.getContext(), varName)});
                        // errs() << *callInst << "\n";

                        Value *strPointer = builder.CreateGlobalStringPtr(varName);
                        //Using a Vector instead of ArrayRef
                         const std::vector<llvm::Value *> args{strPointer};
                         builder.CreateCall(exitFunc, args);

                    }
                }

                */

            }
        }
    }
    return false;
}
static RegisterPass<SkeletonPass> SKELETONPASS("skeleton", "It is skeleton pass");
