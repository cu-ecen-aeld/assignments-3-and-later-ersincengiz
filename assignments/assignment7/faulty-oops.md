# Faulty dev analysis

## When issuing the below command to /dev/faulty

`echo "hello_world" > /dev/faulty`

## The following exception was received:

### See highlighted sections inline to locate the faulty line in the kernel driver
#### Error type: "Unable to handle kernel NULL pointer dereference" - This means that the kernel encountered a NULL pointer (a pointer that does not point to any valid memory location) and tried to access it, resulting in an error.

```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000

```
#### Virtual address: "0000000000000000" - The virtual address where the error occurred is at address 0x0000000000000000, which is a NULL pointer.
#### Above indicates a null pointer similar to a segmentation fault in user space

```
echo "Hello_world" > /dev/faulty 
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000420b8000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 149 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
```

#### Above "pc" program counter indicates a fault write 0x14 bytes into the function which is 0x20 bytes long

```
lr : vfs_write+0xa8/0x2a0
sp : ffffffc008cfbd80
x29: ffffffc008cfbd80 x28: ffffff80020e8000 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 000000000000000c x21: 0000005593343980
x20: 0000005593343980 x19: ffffff80020c6c00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f7000 x3 : ffffffc008cfbdf0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0x100
 do_el0_svc+0x44/0xb0
 el0_svc+0x28/0x80
 el0t_64_sync_handler+0xa4/0x130
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 5dc9d5952439e1e4 ]---
```
```
pc : faulty_write+0x14/0x20 [faulty]

```
##### From this line, we find out that the program counter was at function "faulty_write", 0x14 bytes into the function, which is 0x20 long



#### The first line of oops message hinted that there was NULL pointer dereferencing.
#### The register pc pointed to the location of code that caused the error which in this case is 0x14 bytes in faulty_write #### function in faulty module.
#### By running buildroot/output/host/bin/aarch64-buildroot-linux-gnu-objdump -S buildroot/output/target/lib/modules/5.15.18/extra/faulty.ko 

```
buildroot/output/target/lib/modules/5.15.18/extra/faulty.ko:     file format elf64-littleaarch64


Disassembly of section .text:

0000000000000000 <faulty_write>:
   0:	d503245f 	bti	c
   4:	d2800001 	mov	x1, #0x0                   	// #0
   8:	d2800000 	mov	x0, #0x0                   	// #0
   c:	d503233f 	paciasp
  10:	d50323bf 	autiasp
  14:	b900003f 	str	wzr, [x1]
  18:	d65f03c0 	ret
  1c:	d503201f 	nop

0000000000000020 <faulty_read>:
  20:	d503233f 	paciasp


```
##### The routine faulty_write is clearly 0x20 bytes long (starts at 0x0, last instruction at 0x1c, and faulty_read starts at 0x20).
##### The register x1 is loaded with 0 at instruction address 0x4
##### The output of objdump of faulty.ko shows that at 0x14 offset of faulty_write function, there is a write to address 0 pointed by register x1.