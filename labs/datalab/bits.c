/*
 * @Author: Li Junjie
 * @Description: 
 */
/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y)
{
  /* Use truth talbe, sum-of-products and De Morgan's Laws */
  return ~(~(~x & y) & ~(x & ~y));
}

/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void)
{
  return 1 << 31;
}

//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x)
{
  // Tmax + 1 = ~Tmax, use x ^ y = 0 to justify x == y
  // exclude -1 because (-1 + 1) = ~(-1), use -1 + 1 = 0 to exclude -1
  // other number a add 1 will not affect the sign bit(most significant bit), 
  // so (a + 1) != ~a
  return !((!(x + 1)) | ((x + 1) ^ ~x));
}

/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x)
{
  // b = 0xAAAAAAAA is to long, use 0xAA to generate
  int a = 0xAA << 8 | 0xAA;
  int b = a << 16 | a;
  return !((x & b) ^ b);
}

/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x)
{
  // x + ~x = -1;  x + -x = 0
  // -x = ~x + 1;
  return ~x + 1;
}

//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x)
{
  /* 
  ** First confirm the number from fourth to the highest is 0x3
  ** Then confirm the number form the least significant to third bit
  ** is lower than 10
  *** Add -10 (~10 + 1), then the most siginificant bit shoule be one (negative number)
  */
  int four2highest = !((x >> 4) ^ 0x03);
  // the !!number covert a number != 0 to 1
  int zero2three = !!(((x & 0x0F) + (~10 + 1)) >> 31);
  return four2highest & zero2three;
}

/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z)
{
  int selecotr = !!x; // selectore = 0 if x == 0 else selector = 1
  selecotr = ~selecotr + 1; // selector = 0xFFFFFFFF if selector !=0 else selector = 0
  return (selecotr & y) | (~selecotr & z);
}

/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y)
{
  int x_sub_y = x + ~y + 1;  // -y = ~y + 1
  int ZF = !x_sub_y;  // zero flag
  int SF = !!(x_sub_y >> 31);  // sign flag
  int x_sign = !!(x >> 31);
  int y_sign = !!(y >> 31);
  int OF = (x_sign ^ y_sign) & (x_sign ^ SF);  // overflow flag
  return (SF ^ OF) | ZF;
}

//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x)
{
  // -x = ~x + 1
  // the most siginificant bit of x | -x will be 0 iff x = 0 otherwise
  // it will be 1
  return ((x | (~x + 1)) >> 31) + 1;
}

/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x)
{
  /*
  * If x > 0, the minimum number of bits required to represent x in two's complement equlas
  * the position of the most significant bit(MSB) add a sign bit.
  * If x < 0, the same is true for ~x 
  * Using the concept of binary search for processing.
  */
  int sign;
  int result;
  int MSB_in_high_order_16_bits;
  int MSB_in_high_order_8_bits;
  int MSB_in_high_order_4_bits;
  int MSB_in_high_order_2_bits;
  int MSB_in_high_order_1_bits;
  int offset;

  result = 1; // at lest need a sign bit.

  sign = !!(x >> 31);
  x = (~sign + 1) ^ x; // if x < 0, will get ~x, if x > 0, will get x 

  MSB_in_high_order_16_bits = !!(x >> 16); 
  // if MSB in higher order 16 bit, offset = 16, else offset = 0     
  offset = MSB_in_high_order_16_bits << 4;  
  result += offset;  
  // if MSB_in_high_order_16_bits, do nothing, else set x to the high-order-16 bits
  x >>= offset;  

  // the rest is the same as the 16 bit
  MSB_in_high_order_8_bits = !!(x >> 8);
  offset = MSB_in_high_order_8_bits << 3;
  result += offset;
  x >>= offset;

  MSB_in_high_order_4_bits = !!(x >> 4);
  offset = MSB_in_high_order_4_bits << 2;
  result += offset;
  x >>= offset;

  MSB_in_high_order_2_bits = !!(x >> 2);
  offset = MSB_in_high_order_2_bits << 1;
  result += offset;
  x >>= offset;

  MSB_in_high_order_1_bits = !!(x >> 1);
  offset = MSB_in_high_order_1_bits;
  result += offset;
  x >>= offset;

  result += x;

  return result;
}

//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf)
{
  unsigned sign, significand, exponent;

  exponent = uf << 1 >> 24;
  if (exponent == 255) /*f is NaN or inf*/
    return uf;

  sign = uf >> 31;
  significand = uf << 9 >> 9;
  if (exponent) /* f is a Normalized Value*/
  {
    exponent += 1;
    if (exponent == 255) /* 2 * f is inf */
      significand = 0;
  }
  else
  {
    /* f is a Denormalized Value */
    if (significand >> 22)
    {
      exponent = 1;  /* need turn it to a Normalized value*/
      significand = significand << 10 >> 9;  /* remove the first bit of significand */
    }
    else {
      significand <<= 1;
    }
  }
  return (sign << 31) + (exponent << 23) + significand;
}

/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf)
{
  unsigned sign, signicificand, exponent;
  int offset;

  exponent = uf << 1 >> 24;
  if (exponent >= 158) /* 158 = 127 + 31 */
    return 1 << 31;    /*0x80000000*/
  if (exponent < 127)  /* value < 1 */
    return 0;
  
  /* otherwise uf represent a normal value */
  sign = uf >> 31;
  signicificand = (uf << 9 >> 9) | (1 << 23);
  /* if exponet = 127 + 23 = 150, sigicificand will be the absolute value.*/
  offset = exponent - 150;
  if (offset <= 0)
    signicificand >>= -offset;
  else
    signicificand <<= offset;

  if (sign)
    return ~signicificand + 1;
  return signicificand;
}

/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x)
{

  if (x < -149) /* to small -149 = -126 - 23 */
    return 0;

  if (x >= 128) /* +inf 128 = 127 + 1 */
    return 255 << 23;  /* exponet = 255, all 1,  +INF */

  /* denormal */
  if (x <= -127 && x >= -149)  /*exponet = 0*/
    return 1 << (149 + x);

  /* normal */
  return (x + 127) << 23;  /* siginifcand = 0 */
}
