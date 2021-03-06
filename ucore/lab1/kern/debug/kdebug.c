#include <defs.h>
#include <x86.h>
#include <stab.h>
#include <stdio.h>
#include <string.h>
#include <kdebug.h>

#define STACKFRAME_DEPTH 20

extern const struct stab __STAB_BEGIN__[];  // beginning of stabs table # stabs表的开始位�
//  stab是个啥玩意
extern const struct stab __STAB_END__[];    // end of stabs table # stabs表的结束位置
extern const char __STABSTR_BEGIN__[];      // beginning of string table # string表的开始位
extern const char __STABSTR_END__[];        // end of string table

/* debug information about a particular instruction pointer */
struct eipdebuginfo {
	// 源文件名
    const char *eip_file;                   // source code filename for eip
    int eip_line;                           // source code line number for eip
    const char *eip_fn_name;                // name of function containing eip
    int eip_fn_namelen;                     // length of function's name
	// 函数的开始地址
    uintptr_t eip_fn_addr;                  // start address of function
    int eip_fn_narg;                        // number of function arguments
};

/* *
 * stab_binsearch - according to the input, the initial value of
 * range [*@region_left, *@region_right], find a single stab entry
 * that includes the address @addr and matches the type @type,
 * and then save its boundary to the locations that pointed
 * by @region_left and @region_right.
 *
 * Some stab types are arranged in increasing order by instruction address.
 * For example, N_FUN stabs (stab entries with n_type == N_FUN), which
 * mark functions, and N_SO stabs, which mark source files.
 * 一些stab types按照指令地址递增的顺序来安排
 *
 * Given an instruction address, this function finds the single stab entry
 * of type @type that contains that address.
 *
 * 给定一个指令的地址，这个函数找到对应的单个stab entry，并且是type类型� * The search takes place within the range [*@region_left, *@region_right].
 * Thus, to search an entire set of N stabs, you might do:
 *
 *      left = 0;
 *      right = N - 1;    (rightmost stab)
 *      stab_binsearch(stabs, &left, &right, type, addr);
 *
 * The search modifies *region_left and *region_right to bracket the @addr.
 * *@region_left points to the matching stab that contains @addr,
 * and *@region_right points just before the next stab.
 * If *@region_left > *region_right, then @addr is not contained in any
 * matching stab.
 * 这个搜索修改 *region_left�region_right两个边界的� *
 * For example, given these N_SO stabs:
 *      Index  Type   Address
 *      0      SO     f0100000
 *      13     SO     f0100040
 *      117    SO     f0100176
 *      118    SO     f0100178
 *      555    SO     f0100652
 *      556    SO     f0100654
 *      657    SO     f0100849
 * this code:
 *      left = 0, right = 657;
 *      stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
 * will exit setting left = 118, right = 554.
 * 好吧，讲的已经很详细了! * */
static void
stab_binsearch(const struct stab *stabs, int *region_left, int *region_right,
           int type, uintptr_t addr) {
	// stabs就是这样的一个个的stab集合
    int l = *region_left, r = *region_right, any_matches = 0;

    while (l <= r) {
		// true middle这可是真正的中间啊！
        int true_m = (l + r) / 2, m = true_m; // 好吧，很明显，用的是二分�
        // search for earliest stab with right type
        while (m >= l && stabs[m].n_type != type) {
            m --; // 在左边寻找，找到类型一致的
        }
        
        if (m < l) {    // no match in [l, m]
            l = true_m + 1; 
            continue;
        }
	// 一般来说，执行到了这一步的话，肯定是找到了类型一致的，并且m >= 1

        any_matches = 1; // 然后是真正的二分法
        if (stabs[m].n_value < addr) { // 可以猜测n_value必定是地址
            *region_left = m; // 我们的目标是什么，是让region_left和region_right包围addr
            l = true_m + 1;
        } else if (stabs[m].n_value > addr) {
            *region_right = m - 1;
            r = m - 1;
        } else {
	    // 到这里的话，说明stabs[m].n_value == addr
            // exact match for 'addr', but continue loop to find
            // *region_right
            *region_left = m;
            l = m;
            addr ++;
        }
    }

    if (!any_matches) { // 到这里的话，说明一个都没有匹配
        *region_right = *region_left - 1;
    }
    else {
        // find rightmost region containing 'addr'
        l = *region_right;
        for (; l > *region_left && stabs[l].n_type != type; l --)
            /* do nothing */;
        *region_left = l;
    }
}

/* *
 * debuginfo_eip - Fill in the @info structure with information about
 * the specified instruction address, @addr.  Returns 0 if information
 * was found, and negative if not.  But even if it returns negative it
 * has stored some information into '*info'.
 * */
 
 // 好吧，具体而言就是填充*info,根据地址addr
int
debuginfo_eip(uintptr_t addr, struct eipdebuginfo *info) {
    const struct stab *stabs, *stab_end;
    const char *stabstr, *stabstr_end;

    info->eip_file = "<unknown>";
    info->eip_line = 0;
    info->eip_fn_name = "<unknown>";
    info->eip_fn_namelen = 9; // 指的�unknown>的长度是9
    info->eip_fn_addr = addr; // 函数的地址信息
    info->eip_fn_narg = 0; // 参数信息

    stabs = __STAB_BEGIN__;
    stab_end = __STAB_END__;
    stabstr = __STABSTR_BEGIN__;
    stabstr_end = __STABSTR_END__;

    // String table validity checks
	// 很显而易见的验证，是吧！
    if (stabstr_end <= stabstr || stabstr_end[-1] != 0) {
        return -1;
    }

    // Now we find the right stabs that define the function containing
    // 'eip'.  First, we find the basic source file containing 'eip'.
    // Then, we look in that source file for the function.  Then we look
    // for the line number.

    // Search the entire set of stabs for the source file (type N_SO).
	// 首先是要找到对应的文件名，是吧！
    int lfile = 0, rfile = (stab_end - stabs) - 1; // rfile指的是大小？
	// 感觉是要寻找文件名的样子
	// 注意一下，这个stabs是stabs表的第一xiang
    stab_binsearch(stabs, &lfile, &rfile, N_SO, addr); // 开始二分查zhao
    if (lfile == 0) // 也就是没有找到相关的信息
        return -1;

    // Search within that file's stabs for the function definition
    // (N_FUN).
    // 然后是找到对应的函数
    int lfun = lfile, rfun = rfile;
    int lline, rline;
    stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

    if (lfun <= rfun) {
        // stabs[lfun] points to the function name
        // in the string table, but check bounds just in case.
        if (stabs[lfun].n_strx < stabstr_end - stabstr) {
            info->eip_fn_name = stabstr + stabs[lfun].n_strx;
        }
        info->eip_fn_addr = stabs[lfun].n_value;
        addr -= info->eip_fn_addr;
        // Search within the function definition for the line number.
        lline = lfun;
        rline = rfun;
    } else {
        // Couldn't find function stab!  Maybe we're in an assembly
        // file.  Search the whole file for the line number.
        info->eip_fn_addr = addr;
        lline = lfile;
        rline = rfile;
    }
    info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;

    // Search within [lline, rline] for the line number stab.
    // If found, set info->eip_line to the right line number.
    // If not found, return -1.
    stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
    if (lline <= rline) {
        info->eip_line = stabs[rline].n_desc;
    } else {
        return -1;
    }

    // Search backwards from the line number for the relevant filename stab.
    // We can't just use the "lfile" stab because inlined functions
    // can interpolate code from a different file!
    // Such included source files use the N_SOL stab type.
    while (lline >= lfile
           && stabs[lline].n_type != N_SOL
           && (stabs[lline].n_type != N_SO || !stabs[lline].n_value)) {
        lline --;
    }
    if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr) {
        info->eip_file = stabstr + stabs[lline].n_strx;
    }

    // Set eip_fn_narg to the number of arguments taken by the function,
    // or 0 if there was no containing function.
    if (lfun < rfun) {
        for (lline = lfun + 1;
             lline < rfun && stabs[lline].n_type == N_PSYM;
             lline ++) {
            info->eip_fn_narg ++;
        }
    }
    return 0;
}

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void
print_kerninfo(void) {
    extern char etext[], edata[], end[], kern_init[];
    cprintf("Special kernel symbols:\n");
    cprintf("  entry  0x%08x (phys)\n", kern_init);
    cprintf("  etext  0x%08x (phys)\n", etext);
    cprintf("  edata  0x%08x (phys)\n", edata);
    cprintf("  end    0x%08x (phys)\n", end);
    cprintf("Kernel executable memory footprint: %dKB\n", (end - kern_init + 1023)/1024);
}

/* *
 * print_debuginfo - read and print the stat information for the address @eip,
 * and info.eip_fn_addr should be the first address of the related function.
 * */
 // 因为eip是某个函数应该执行的下一条指令的地址
 // 所以这个函数的意思是，根据这个地址，结合gdb的调试信息，输出有关于这个函数的一系列信息
void
print_debuginfo(uintptr_t eip) {
    struct eipdebuginfo info; // 记录下debug的一些信息的一个结构
    if (debuginfo_eip(eip, &info) != 0) {
        cprintf("    <unknow>: -- 0x%08x --\n", eip);
    }
    else {
        char fnname[256];
        int j;
        for (j = 0; j < info.eip_fn_namelen; j ++) {
            fnname[j] = info.eip_fn_name[j]; // 复制到fnname，也就是函数的名字吧！
        }
        fnname[j] = '\0';
        cprintf("    %s:%d: %s+%d\n", info.eip_file, info.eip_line,
                fnname, eip - info.eip_fn_addr);
    }
}

static __noinline uint32_t
read_eip(void) {
    uint32_t eip;
    asm volatile("movl 4(%%ebp), %0" : "=r" (eip));
    return eip;
}

/* *
 * print_stackframe - print a list of the saved eip values from the nested 'call'
 * instructions that led to the current point of execution
 *
 * The x86 stack pointer, namely esp, points to the lowest location on the stack
 * that is currently in use. Everything below that location in stack is free. Pushing
 * a value onto the stack will invole decreasing the stack pointer and then writing
 * the value to the place that stack pointer pointes to. And popping a value do the
 * opposite.
 * 
 * ebp是基址指针
 * The ebp (base pointer) register, in contrast, is associated with the stack
 * primarily by software convention. On entry to a C function, the function's
 * prologue code normally saves the previous function's base pointer by pushing
 * it onto the stack, and then copies the current esp value into ebp for the duration
 * of the function. If all the functions in a program obey this convention,
 * then at any given point during the program's execution, it is possible to trace
 * back through the stack by following the chain of saved ebp pointers and determining
 * exactly what nested sequence of function calls caused this particular point in the
 * program to be reached. This capability can be particularly useful, for example,
 * when a particular function causes an assert failure or panic because bad arguments
 * were passed to it, but you aren't sure who passed the bad arguments. A stack
 * backtrace lets you find the offending function.
 *
 * The inline function read_ebp() can tell us the value of current ebp. And the
 * non-inline function read_eip() is useful, it can read the value of current eip,
 * since while calling this function, read_eip() can read the caller's eip from
 * stack easily.
 *
 * In print_debuginfo(), the function debuginfo_eip() can get enough information about
 * calling-chain. Finally print_stackframe() will trace and print them for debugging.
 *
 * Note that, the length of ebp-chain is limited. In boot/bootasm.S, before jumping
 * to the kernel entry, the value of ebp has been set to zero, that's the boundary.
 * */
void
print_stackframe(void) {
     /* LAB1 YOUR CODE : STEP 1 */
     /* (1) call read_ebp() to get the value of ebp. the type is (uint32_t);
      * (2) call read_eip() to get the value of eip. the type is (uint32_t);
      * (3) from 0 .. STACKFRAME_DEPTH
      *    (3.1) printf value of ebp, eip
      *    (3.2) (uint32_t)calling arguments [0..4] = the contents in address (unit32_t)ebp +2 [0..4]
      *    (3.3) cprintf("\n");
      *    (3.4) call print_debuginfo(eip-1) to print the C calling function name and line number, etc.
      *    (3.5) popup a calling stackframe
      *           NOTICE: the calling funciton's return addr eip  = ss:[ebp+4]
      *                   the calling funciton's ebp = ss:[ebp]
      */
	  uint32_t ebp = read_ebp(); // ebp指的是帧指针，也就是指示着这个函数堆栈开始的地址
	  uint32_t eip = read_eip(); // 就是用于输出对应的值
	  int i, j;
	  // 上面的提示已经告诉了我们界限了，那就是一旦ebp为0，那么就应该结束循环
	  for (i = 0; ebp != 0 && i < STACKFRAME_DEPTH; i++) {
		  cprintf("ebp:0x%08x eip:0x%08x args:", ebp, eip);
		  uint32_t *args = (uint32_t *)ebp + 2;
		  for (j = 0; j < 4; j++) {
			  cprintf("0x%08x ", args[j]); // 栈有个特别有意思的特性就是，它是向下生长的
		  }
		  cprintf("\n");
		  print_debuginfo(eip - 1);
		  // 我们来展示一下调用一个函数的时候是怎样来压栈的吧！
		  // 首先是将参数压栈
		  // 然后将eip压栈，因为eip中存放了在当前函数中将要执行的下一条指令的值
		  // 然后将ebp压栈，这里面存放了上一个caller（调用这个函数的函数）的基地址(栈内)
		  // 然后将esp的值赋给ebp，然后加载新函数的偏移地址进入eip，开始执行
		  eip = ((uint32_t *)ebp)[1]; // 也就是说，先压入eip，再压入ebp，是吧！
		  ebp = ((uint32_t *)ebp)[0]; // ebp指向的
	  }
}

