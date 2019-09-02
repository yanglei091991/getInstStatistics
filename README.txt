How to get ucp instruction statistic:
1. Untar AsmParser-getInstStatistic.tar.gz, and get AsmParser directory.
2. Copy AsmParser directory to: $LLVM_SOURCE_PATH/lib/Target/UCPM/,
     to replace preivous code.
3. Compile llvm and get llvm-mc.
4. Adjust target file "source.mpu.s":
   4.1 Use ".hmacro XXX" and ".endhmacro" to embody all the ucp instructions
         in source.mpu.s
   4.2 Repalce all "||" symols to "\r\n;"; In vim execute: %s/||/\n\r;/g
4. Run: llvm-mc -arch=ucpm -filetype=obj -o test.o source.mpu.s > dis.txt
5. Adjust file dis.txt and remove redundant instructions.
     In vim execute: %s/||.*$//g

