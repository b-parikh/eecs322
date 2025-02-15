#include <iostream>
#include <string>
#include <analysis.h>

namespace L2 {

   set_of_str CALLER_SAVED_REGISTERS =
     {"r8", "r9", "r10", "r11", "rax", "rcx", "rdi", "rdx", "rsi"};
   set_of_str CALLEE_SAVED_REGISTERS =
     {"r12", "r13", "r14", "r15", "rbp", "rbx"};
   vector_of_str ARGUMENT_REGISTERS =
     {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

   void print_set(set_of_str &s) {
       for(std::string const i : s)
           std::cout << i  << ' ';
       std::cout << '\n';
   }

   void print_instruction(Instruction &instruction) {
        for(Item i : instruction.items) {
            std::cout << i.labelName << ' ';
        }
        std::cout << '\n';
   }

   set_of_str subtract_sets(set_of_str s1, set_of_str s2) { // out - kill
        std::vector<std::string> toDelete;

        for(auto &i : s1) {
            set_of_str::const_iterator item = s2.find(i);
            if(item != s2.end()) { // if item is in s2
                toDelete.push_back(i);
            }
        }

        for(auto &i : toDelete){
            set_of_str::const_iterator item = s1.find(i);
            s1.erase(item);
        }

        return s1;
   }

    bool regOrVar(Item i) {
        if(i.type == Type::reg || i.type == Type::var)
            return true;
        return false;
    }

    void init_all_sets(Function* fp) {
        for(auto i : fp->instructions) {
            i->gen_set.clear();
            i->kill_set.clear();
            i->in_set.clear();
            i->out_set.clear();
        }
    }
    /*
     * Gets the function and the index of the instruction of which the successor
     * needs to be found.
     */
    void goto_successor_push_back(Function &f, int currInstructionIndex) {
        Instruction* currI = f.instructions[currInstructionIndex];
        std::string labelToMatch = currI->items[1].labelName;
        for(int n = 0; n < f.instructions.size(); ++n) {
            Instruction* i = f.instructions[n];
            if(i->identifier != 10) // if not label_instruction
                continue;
            else {
                if(i->items[0].labelName == labelToMatch)
                    currI->successors.push_back(f.instructions[n+1]);
                else
                    continue;
            }
        }
    }

    /*
     * Gets the function and the index of the instruction of which the successor
     * needs to be found.
     */
    void cjump_onearg_successor_push_back(Function &f, int currInstructionIndex) {
        Instruction* prevI = f.instructions[currInstructionIndex];
        std::string labelToMatch = prevI->items[4].labelName;
        prevI->successors.push_back(f.instructions[currInstructionIndex + 1]);
        for(int n = 0; n < f.instructions.size(); ++n) {
            Instruction* i = f.instructions[n];
            if(i->identifier != 10) // if not label_instruction
                continue;
            else {
                if(i->items[0].labelName == labelToMatch) {
                    prevI->successors.push_back(f.instructions[n+1]); // push back the instruction after the label
                    //std::cout << n << " sucessor found\t";
                    //print_instruction(*f.instructions[n+1]);
                }
                else
                    continue;
            }
        }
    }
        
    void cjump_twoargs_successor_push_back(Function &f, int currInstructionIndex) {
        Instruction* prevI = f.instructions[currInstructionIndex];
        std::string label1 = prevI->items[4].labelName;
        std::string label2 = prevI->items[5].labelName;
        for(int n = 0; n < f.instructions.size(); ++n) {
            Instruction* i = f.instructions[n];
            if(i->identifier != 10) // if not label_instruction
                continue;
            else {
                if(i->items[0].labelName == label1 || i->items[0].labelName == label2)
                    prevI->successors.push_back(f.instructions[n+1]);
                else
                    continue;
            }
        }
    }

    void gk_return(Instruction &i) {
        i.gen_set.emplace("rax");
        i.gen_set.insert(CALLEE_SAVED_REGISTERS.begin(), CALLEE_SAVED_REGISTERS.end());
        // return has no kill set
    }

    void gk_assignment(Instruction &i) {

        set_of_str gen_set = i.gen_set;
        set_of_str kill_set = i.kill_set;

        int instruction_length = (i.items).size();

        if (instruction_length == 3) {
            if(regOrVar(i.items[2])) { //RHS == reg, var
                i.reg_var_assignment = true;
                gen_set.emplace(varNameModifier(i.items[2]));
            }
            kill_set.emplace(varNameModifier(i.items[0]));
        }
        else { // length == 5 (stack arg is elsewhere)
            if(i.items[0].type == Type::mem) { // LHS is mem
                if(i.items[1].labelName != "rsp") // hacky way to check if a register is rsp
                    gen_set.emplace(varNameModifier(i.items[1])); // reg or var
                if(regOrVar(i.items[4])) 
                    gen_set.emplace(varNameModifier(i.items[4]));
            } else {
                if (i.items[3].labelName != "rsp")
                    gen_set.emplace(varNameModifier(i.items[3]));
                kill_set.emplace(varNameModifier(i.items[0]));
            }
        }

        i.gen_set = gen_set;
        i.kill_set = kill_set;
      
    }

    void gk_arithmetics(Instruction &i) { // reg1, var1, mem = reg1, var, mem + rev2, var2, mem, num
        set_of_str gen_set = i.gen_set;
        set_of_str kill_set = i.kill_set;

        int instruction_length = (i.items).size();

        if (instruction_length == 3) {
            if(regOrVar(i.items[2])) { //RHS == reg, var
                gen_set.emplace(varNameModifier(i.items[2]));
            }
            // LHS = reg, var
            gen_set.emplace(varNameModifier(i.items[0]));
            kill_set.emplace(varNameModifier(i.items[0]));
        }
        else { // length == 5
            if(i.items[0].type == Type::mem) { // LHS is mem
                gen_set.emplace(varNameModifier(i.items[1])); // reg or var
                if(regOrVar(i.items[4]))
                    gen_set.emplace(varNameModifier(i.items[4]));
            } else {
                gen_set.emplace(varNameModifier(i.items[3]));
                gen_set.emplace(varNameModifier(i.items[0]));
                kill_set.emplace(varNameModifier(i.items[0]));
            }
        }

        i.gen_set = gen_set;
        i.kill_set = kill_set;
    }

    void gk_inc_dec(Instruction &i) { // reg, var = reg, var + 1
        i.gen_set.emplace(varNameModifier(i.items[0]));
        i.kill_set.emplace(varNameModifier(i.items[0]));
    }

    void gk_assign_comparison(Instruction &i) { // reg1, var1 <- reg2, var2, num (compare) reg3, var3, num
        if(regOrVar(i.items[2])) // first t in RHS is not number
            i.gen_set.emplace(varNameModifier(i.items[2]));
        if(regOrVar(i.items[4])) // second t in RHS is not number
            i.gen_set.emplace(varNameModifier(i.items[4]));

        i.kill_set.emplace(varNameModifier(i.items[0]));
    }

    void gk_shift(Instruction &i) { // reg1, var1 <<= reg2, var2
        if(regOrVar(i.items[2])) {
            std::string modifiedLabel = varNameModifier(i.items[2]);
            i.gen_set.emplace(modifiedLabel);
            if(modifiedLabel != "rcx")
                i.shifting_var_or_reg = modifiedLabel;
        }

        i.gen_set.emplace(varNameModifier(i.items[0]));
        i.kill_set.emplace(varNameModifier(i.items[0]));
    }

    void gk_cjump(Instruction &i) {
        if(regOrVar(i.items[1])) // first t in RHS is not number
            i.gen_set.emplace(varNameModifier(i.items[1]));
        if(regOrVar(i.items[3])) // second t in RHS is not number
            i.gen_set.emplace(varNameModifier(i.items[3]));
    }

    void gk_lea(Instruction &i) { // reg1, var1 = reg2, var2 + (reg3, var3 * 4)
        i.gen_set.emplace(varNameModifier(i.items[2])); // items[1] == @
        i.gen_set.emplace(varNameModifier(i.items[3]));

        i.kill_set.emplace(varNameModifier(i.items[0]));
    }

    void gk_call(Instruction &i) { // call (reg, var, label, runtime) num
        if(regOrVar(i.items[1])) {// neither label nor runtime functions
            i.gen_set.emplace(varNameModifier(i.items[1]));
        }

        int num_args = stoi(i.items[2].labelName);
        int num_args_gen = (num_args > 6) ? 6 : num_args;
        for (int inum = 0; inum < num_args_gen; inum++)
            i.gen_set.emplace(ARGUMENT_REGISTERS[inum]);

        i.kill_set = CALLER_SAVED_REGISTERS;
        i.kill_set.emplace("rax");
    }

    void gk_stack_arg(Instruction& i) {
        i.kill_set.emplace(varNameModifier(i.items[0]));
    }

    void compute_gen_and_kill(Instruction &instruct) {
        if(instruct.identifier == 0) { // assignment
            gk_assignment(instruct);
        }
         else if(instruct.identifier == 1)
            gk_return(instruct);
         else if(instruct.identifier == 2)
            gk_arithmetics(instruct);
         else if(instruct.identifier == 3)
            gk_inc_dec(instruct);
         else if(instruct.identifier == 4)
            gk_assign_comparison(instruct);
         else if(instruct.identifier == 5)
            gk_shift(instruct);
//         else if(instruct.identifier == 6)
//            gk_goto(ip, outputFile); // just a label
         else if(instruct.identifier == 7)
            gk_cjump(instruct);
         else if(instruct.identifier == 8)
            gk_lea(instruct);
         else if(instruct.identifier == 9)
            gk_call(instruct);
//         else if(ip->identifier == 10)
//            write_label_instruction(ip,outputFile);
         else if(instruct.identifier == 11)
            gk_cjump(instruct);
        else if(instruct.identifier == 12)
            gk_stack_arg(instruct);
         else
            ;   // Do nothing for GOTO and LABEL_INSTRUCTION

    }

    void compute_in_and_out(Function &f) {
        // computing in and out set going from last to first instruction
        int num_instructions = f.instructions.size();
        bool not_converged;
        do{
			not_converged = false;
            // compute IN set of last instruction; has no successor or OUT set
            Instruction* ip = f.instructions[num_instructions - 1];
            ip->in_set = ip->gen_set;

//            std::cout << "\nIN and OUT sets for instruction\n";
//            std::cout << "IN set: ";
//            print_set(ip->in_set);
//            std::cout << "OUT set: ";
//            print_set(ip->out_set);

            // starting from second to last instruction to first instruction, assign successor(s)
            for(int n = num_instructions - 2; n >= 0; --n) {
                ip = f.instructions[n];
                if(ip->identifier == 6) { // push_back successor if goto 
                   goto_successor_push_back(f, n); 
                }
                else if(ip->identifier == 11) { // push_back successor if cjump_onearg
                        cjump_onearg_successor_push_back(f, n);
                } 
                else if(ip->identifier == 7) { // push_back successors if cjump_twoargs
                    cjump_twoargs_successor_push_back(f, n);
                }
                else
                    ip->successors.push_back(f.instructions[n+1]);

                ip->prev_out_set = ip->out_set;
                ip->prev_in_set = ip->in_set;
                
                //std::cout << "curr out_set: ";
                //print_set(ip->out_set);
                // foreach successor, insert successor's IN set into currI's OUT set (populate out set)
                ip->out_set.clear();
                if(ip->identifier != 1) {
                    for(auto sp : ip->successors) { // return should have no out set
                        (ip->out_set).insert(sp->in_set.begin(), sp->in_set.end());
                    //std::cout << "an in_set placed into curr out_set: ";
                    //print_set(sp->in_set);
                    }
                }
                //std::cout << "out_set after insertions: ";
                //print_set(ip->out_set);
                // populate in set
//                std::cout << "OUT and KILL sets\n";
//                std::cout << "OUT set: ";
//                print_set(ip->out_set);
//                std::cout << "KILL set: ";
//                print_set(ip->kill_set);
                set_of_str out_sub_kill = subtract_sets(ip->out_set, ip->kill_set); // OUT[i] - KILL[i]
                ip->in_set.clear();
                ip->in_set.insert(out_sub_kill.begin(), out_sub_kill.end());
                ip->in_set.insert(ip->gen_set.begin(), ip->gen_set.end());

                if(ip->prev_out_set != ip->out_set || ip->prev_in_set != ip->in_set) {
                    not_converged = true;
                }
//            std::cout << "\nIN and OUT sets for instruction\n";
//            std::cout << "IN set: ";
//            print_set(ip->in_set);
//            std::cout << "OUT set: ";
//            print_set(ip->out_set);
//            std::cout << '\n';
            }
        } while(not_converged);

    }

    void analyze(Function* fp) {
        Function f = *fp;

        init_all_sets(fp);
        //std::cout << "Function: " << f.name << '\n';
        for(Instruction* instruct : f.instructions) {
            compute_gen_and_kill(*instruct);
            // print gen and kill for each instruction
//            std::cout << "instruction: \ngen_set: ";
//            print_set(instruct->gen_set);
//            std::cout << "kill_set: ";
//            print_set(instruct->kill_set);         
//            std::cout << '\n';
	    }
        
        compute_in_and_out(f);

//        if(liveness_only) {
//            std::cout << "(\n(in\n";
//            for(auto i : f.instructions) {
//                std::cout << " (";
//                for(auto s : i->in_set) {
//                    std::cout << s << ' ';
//                }
//                std::cout << ")\n";
//            }
//            std::cout << ")\n\n(out\n";
//            for(auto i : f.instructions) {
//                std::cout << " (";
//                for(auto s : i->out_set)
//                    std::cout << s << ' ';
//                std::cout << ")\n";
//            }
//    		std::cout << ")\n\n)";
//        }

    }


}
