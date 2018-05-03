#include <byteswap.h>

#include <type_traits>
#include <utility>
#include <limits>
#include <new>

#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>

using namespace std;

#if !defined(__BYTE_ORDER) || !defined(__LITTLE_ENDIAN) || (__BYTE_ORDER != __LITTLE_ENDIAN)
	#error "This game is compatible only with little-endian CPUs!"
#endif
#if !defined(__WORDSIZE) || (__WORDSIZE != 32)
	#error "This game is compatible only with 32-bit CPUs!"
#endif

#define ASSERT_SIZE(size,val) \
	static_assert(sizeof(val) == size, "Size of the argument doesn't match!")

#define ASSERT_SIZES(val1,val2) \
	static_assert(sizeof(val1) == sizeof(val2), "Both arguments must have the same size!")

#define SIGNED(val) \
	(make_signed_t<decay_t<decltype(val)>>)val
#define SIGNED_REF(val) \
	(make_signed_t<decay_t<decltype(val)>> &)val

#define UNSIGNED(val) \
	(make_unsigned_t<decay_t<decltype(val)>>)val
#define UNSIGNED_REF(val) \
	(make_unsigned_t<decay_t<decltype(val)>> &)val

#define GET_TYPE(val) \
	decay_t<decltype(val)>

#define Function(type) \
	__attribute__((__noinline__)) type

#define Void \
	Function(void)

#ifdef NDEBUG
	#define InlineFunction(type) \
		__attribute__((__always_inline__)) type
#else
	#define InlineFunction(type) \
		Function(type)
#endif

#define InlineVoid \
	InlineFunction(void)

#define InlineAuto \
	InlineFunction(auto)

#define packed \
	__attribute__((__packed__))

#define ja(operation)  if (!flags.cf && !flags.zf) operation
#define jb(operation)  if (flags.cf) operation
#define jbe(operation) if (flags.cf || flags.zf) operation
#define jg(operation)  if (!flags.zf && (flags.sf == flags.of)) operation
#define jge(operation) if (flags.sf == flags.of) operation
#define jl(operation)  if (flags.sf != flags.of) operation
#define jle(operation) if (flags.zf || (flags.sf != flags.of)) operation
#define jnb(operation) if (!flags.cf) operation
#define jns(operation) if (!flags.sf) operation
#define jnz(operation) if (!flags.zf) operation
#define js(operation)  if (flags.sf) operation
#define jz(operation)  if (flags.zf) operation

struct FPU
{
	FPU()
	{
		init();
	}

	InlineVoid init()
	{
		control.word = 0x037F;
		status.word  = 0x0000;
	}

	/* Store double and float as integers to prevent unaligned memory access, e.g. on VFP */
	InlineVoid store(int64_t &val)
	{
		memcpy(&val, &st(0), 8);
	}
	InlineVoid store(int32_t &val)
	{
		float f = st(0);
		memcpy(&val, &f, 4);
	}

	InlineFunction(double &) st(const int32_t idx)
	{
		return regs[(status.top + idx) & 0x7];
	}

	InlineVoid push(const double val)
	{
		regs[--status.top] = val;
	}
	InlineVoid pop()
	{
		++status.top;
	}

	InlineVoid compare(const double val1, const double val2)
	{
		if (val1 == val2)
		{
			status.c3 = true;
			status.c2 = false;
			status.c0 = false;
		}
		else if (val1 > val2)
		{
			status.c3 = false;
			status.c2 = false;
			status.c0 = false;
		}
		else if (val1 < val2)
		{
			status.c3 = false;
			status.c2 = false;
			status.c0 = true;
		}
		else
		{
			status.c3 = true;
			status.c2 = true;
			status.c0 = true;
		}
	}

	double regs[8]; //Real x87 has 80-bit (long double) registers

	union
	{
		uint16_t word;
		struct
		{
			uint8_t im      : 1;
			uint8_t dm      : 1;
			uint8_t zm      : 1;
			uint8_t om      : 1;
			uint8_t um      : 1;
			uint8_t pm      : 1;
			uint8_t unused1 : 2;
			uint8_t pc      : 2;
			uint8_t rc      : 2;
			uint8_t ic      : 1;
			uint8_t unused2 : 3;
		};
	} control;

	union
	{
		uint16_t word;
		struct
		{
			uint8_t ie  : 1;
			uint8_t de  : 1;
			uint8_t ze  : 1;
			uint8_t oe  : 1;
			uint8_t ue  : 1;
			uint8_t pe  : 1;
			uint8_t sf  : 1;
			uint8_t es  : 1;
			uint8_t c0  : 1;
			uint8_t c1  : 1;
			uint8_t c2  : 1;
			uint8_t top : 3;
			uint8_t c3  : 1;
			uint8_t b   : 1;
		};
	} status;
};

struct CPU
{
	CPU()
	{
		const size_t stack_size = 1 << 20;
		stack = (uint8_t *)malloc(stack_size);
		esp = (int32_t)stack + stack_size;
	}
	~CPU()
	{
		free(stack);
	}

	uint8_t *stack;

	/* edx and ecx are swapped */
	union
	{
		int64_t edx_eax = 0;
		struct
		{
			union
			{
				char *eax_ptr;
				int32_t eax;
				int16_t  ax;
				struct
				{
					int8_t al;
					int8_t ah;
				};
			};
			union
			{
				char *edx_ptr;
				int32_t edx;
				int16_t  dx;
				struct
				{
					int8_t dl;
					int8_t dh;
				};
			};
		};
	};
	union { char *ecx_ptr; int32_t ecx = 0; int16_t  cx; struct { int8_t cl; int8_t ch; }; };
	union { char *ebx_ptr; int32_t ebx = 0; int16_t  bx; struct { int8_t bl; int8_t bh; }; };
	union { char *esp_ptr; int32_t esp = 0; int16_t  sp; };
	union { char *ebp_ptr; int32_t ebp = 0; int16_t  bp; };
	union { char *esi_ptr; int32_t esi = 0; int16_t  si; };
	union { char *edi_ptr; int32_t edi = 0; int16_t  di; };

	int16_t cs = 0, ds = 0, es = 0, fs = 0, gs = 0, ss = 0;

	union Flags
	{
		uint16_t word = 0x0202;
		struct
		{
			uint8_t lo;
			uint8_t hi;
		};
		struct
		{
			/* lo */
			uint8_t cf      : 1;
			uint8_t unused1 : 1; //1
			uint8_t pf      : 1;
			uint8_t unused2 : 1;
			uint8_t af      : 1;
			uint8_t unused3 : 1;
			uint8_t zf      : 1;
			uint8_t sf      : 1;
			/* hi */
			uint8_t unused4 : 1;
			uint8_t unused5 : 1; //1
			uint8_t df      : 1;
			uint8_t of      : 1;
			uint8_t unused6 : 4;
		};
	} flags;

	FPU fpu;
};

struct Application : public CPU
{
	/* AF and PF flags are ignored */

	template<typename T>
	static InlineAuto getMSB(const T val)
	{
		return UNSIGNED(val) >> ((sizeof(val) << 3) - 1);
	}
	template<typename T>
	static InlineAuto getLSB(const T val)
	{
		return val & 1;
	}

	template<typename T>
	static InlineFunction(int64_t &) to64i(T addr)
	{
		return *(int64_t *)addr;
	}
	template<typename T>
	static InlineFunction(int32_t &) to32i(T addr)
	{
		return *(int32_t *)addr;
	}
	template<typename T>
	static InlineFunction(int16_t &) to16i(T addr)
	{
		return *(int16_t *)addr;
	}
	template<typename T>
	static InlineFunction(int8_t &) to8i(T addr)
	{
		return *(int8_t *)addr;
	}

	/* Copy double or float to prevent unaligned memory access, e.g. by VFP */
	template<typename T>
	static InlineFunction(double) get64f(T addr)
	{
		double f;
		memcpy(&f, addr, 8);
		return f;
	}
	template<typename T>
	static InlineFunction(float) get32f(T addr)
	{
		float f;
		memcpy(&f, addr, 4);
		return f;
	}

	template<typename T>
	InlineVoid call(const T addr)
	{
		esp -= 4;
		((void(*)(void *))addr)(this);
		esp += 4;
	}

	template<typename T>
	InlineVoid set_ZF_SF_flags(const T val)
	{
		flags.zf = !val;
		flags.sf = getMSB(val);
	}
	template<bool cf = true, typename T1, typename T2, typename T3>
	InlineVoid set_CF_OF_flags_add(const T1 val1, const T2 val2, const T3 result)
	{
		ASSERT_SIZES(val1, result);
		ASSERT_SIZES(val1, val2);
		if (cf)
			flags.cf = UNSIGNED(result) < UNSIGNED(val1);
		flags.of = (getMSB(val1) != getMSB(result)) && (getMSB(val1) == getMSB(val2));
	}
	template<bool cf = true, typename T1, typename T2, typename T3>
	InlineVoid set_CF_OF_flags_sub(const T1 val1, const T2 val2, const T3 result)
	{
		ASSERT_SIZES(val1, val2);
		ASSERT_SIZES(val1, result);
		if (cf)
			flags.cf = UNSIGNED(val1) < UNSIGNED(val2);
		flags.of = (getMSB(val1) != getMSB(result)) && (getMSB(val1) != getMSB(val2));
	}
	InlineVoid clr_CF_OF_flags()
	{
		flags.of = flags.cf = 0;
	}

	template<typename T>
	InlineVoid push32(const T val)
	{
		ASSERT_SIZE(4, val);
		esp -= 4;
		to32i(esp) = val;
	}
	template<typename T>
	InlineVoid push16(const T val)
	{
		ASSERT_SIZE(2, val);
		esp -= 4;
		to32i(esp) = UNSIGNED(val);
	}

	template<typename T>
	InlineVoid pop32(T &val)
	{
		ASSERT_SIZE(4, val);
		val = to32i(esp);
		esp += 4;
	}
	template<typename T>
	InlineVoid pop16(T &val)
	{
		ASSERT_SIZE(2, val);
		val = to32i(esp);
		esp += 4;
	}

	Void pusha()
	{
		const int32_t tmp = esp;
		push32(eax);
		push32(ecx);
		push32(edx);
		push32(ebx);
		push32(tmp);
		push32(ebp);
		push32(esi);
		push32(edi);
	}
	Void popa()
	{
		int32_t tmp;
		pop32(edi);
		pop32(esi);
		pop32(ebp);
		pop32(tmp);
		pop32(ebx);
		pop32(edx);
		pop32(ecx);
		pop32(eax);
		esp = tmp;
	}

	InlineVoid pushf()
	{
		push16(flags.word);
	}
	InlineVoid popf()
	{
		pop16(flags.word);
	}

	Void leave()
	{
		esp = ebp;
		pop32(ebp);
	}

	InlineVoid sahf()
	{
		flags.lo = (ah & 0xD7) | 0x02;
	}
	InlineVoid lahf()
	{
		ah = (flags.lo & 0xD7) | 0x02;
	}

	InlineVoid setnbe(int8_t &val)
	{
		val = (!flags.cf && !flags.zf);
	}

	template<bool cf = true, typename T1, typename T2>
	Void add(T1 &val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		const GET_TYPE(val1) before = val1;
		val1 += val2;
		set_CF_OF_flags_add<cf>(before, val2, val1);
		set_ZF_SF_flags(val1);
	}
	template<bool cf = true, typename T1, typename T2>
	Void sub(T1 &val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		const GET_TYPE(val1) before = val1;
		val1 -= val2;
		set_CF_OF_flags_sub<cf>(before, val2, val1);
		set_ZF_SF_flags(val1);
	}

	template<typename T>
	InlineVoid inc(T &val)
	{
		add<false>(val, (GET_TYPE(val))1);
	}
	template<typename T>
	InlineVoid dec(T &val)
	{
		sub<false>(val, (GET_TYPE(val))1);
	}

	template<typename T1, typename T2>
	InlineVoid adc(T1 &val1, const T2 val2)
	{
		add(val1, val2 + flags.cf);
	}
	template<typename T1, typename T2>
	InlineVoid sbb(T1 &val1, const T2 val2)
	{
		sub(val1, val2 + flags.cf);
	}

	template<typename T1, typename T2>
	Void and_(T1 &val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		val1 &= val2;
		set_ZF_SF_flags(val1);
		clr_CF_OF_flags();
	}
	template<typename T1, typename T2>
	Void or_(T1 &val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		val1 |= val2;
		set_ZF_SF_flags(val1);
		clr_CF_OF_flags();
	}
	template<typename T1, typename T2>
	Void xor_(T1 &val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		val1 ^= val2;
		set_ZF_SF_flags(val1);
		clr_CF_OF_flags();
	}

	template<typename T1, typename T2>
	Void cmp(const T1 val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		const T1 val = val1 - val2;
		set_ZF_SF_flags(val);
		set_CF_OF_flags_sub(val1, val2, val);
	}
	template<typename T1, typename T2>
	Void test(const T1 val1, const T2 val2)
	{
		ASSERT_SIZES(val1, val2);
		const T1 val = val1 & val2;
		set_ZF_SF_flags(val);
		clr_CF_OF_flags();
	}

	Void div8(uint8_t val)
	{
		const uint8_t quotient  = UNSIGNED(ax) / val;
		const uint8_t remainder = UNSIGNED(ax) % val;
		ah = remainder;
		al = quotient;
	} //flags undefined, no exceptions
	Void div32(uint32_t val)
	{
		const uint32_t quotient  = UNSIGNED(edx_eax) / val;
		const uint32_t remainder = UNSIGNED(edx_eax) % val;
		edx = remainder;
		eax = quotient;
	} //flags undefined, no exceptions
	Void idiv32(int32_t val)
	{
		const int32_t quotient  = edx_eax / val;
		const int32_t remainder = edx_eax % val;
		edx = remainder;
		eax = quotient;
	} //flags undefined, no exceptions

	//TODO: OF and CF flags in mul probably are not needed
	InlineVoid mul32(const uint32_t val)
	{
		edx_eax = (uint64_t)UNSIGNED(eax) * (uint64_t)val;
	}
	InlineVoid imul32(const int32_t val)
	{
		edx_eax = (int64_t)eax * (int64_t)val;
	}
	InlineVoid imul32(int32_t &val1, const int32_t val2)
	{
		val1 *= val2;
	}
	InlineVoid imul32(int32_t &val1, const int32_t val2, const int32_t val3)
	{
		val1 = val2 * val3;
	}
	InlineVoid imul16(int16_t &val1, const int16_t val2)
	{
		val1 *= val2;
	}

	template<typename T1, typename T2>
	Void shr(T1 &val1, const T2 val2)
	{
		if (val2 == 1)
			flags.of = getMSB(val1);
		flags.cf = ((UNSIGNED(val1)) >> (val2 - 1)) & 1;
		UNSIGNED_REF(val1) >>= val2;
		set_ZF_SF_flags(val1);
	}
	template<typename T1, typename T2>
	Void sar(T1 &val1, const T2 val2)
	{
		if (val2 == 1)
			flags.of = 0;
		flags.cf = ((SIGNED(val1)) >> (val2 - 1)) & 1;
		SIGNED_REF(val1) >>= val2;
		set_ZF_SF_flags(val1);
	}
	template<typename T1, typename T2>
	Void shl(T1 &val1, const T2 val2)
	{
		flags.cf =  (UNSIGNED(val1) >> ((sizeof(val1) << 3) - val2)) & 1;
		val1 <<= val2;
		if (val2 == 1)
			flags.of = (flags.cf != getMSB(val1));
		set_ZF_SF_flags(val1);
	}

	template<typename T1, typename T2>
	InlineVoid rol(T1 &val1, const T2 val2)
	{
		val1 = (val1 << val2) | (UNSIGNED(val1) >> ((sizeof val1 << 3) - val2));
	} //TODO: OF, CF, ZF, SF but not needed
	template<typename T1, typename T2>
	InlineVoid ror(T1 &val1, const T2 val2)
	{
		val1 = (UNSIGNED_REF(val1) >> val2) | (val1 << ((sizeof val1 << 3) - val2));
	} //TODO: OF, CF, ZF, SF but not needed

	template<typename T>
	InlineVoid neg(T &val)
	{
//		flags.cf = !!val; //probably not needed
		val = -val;
	} //TODO: OF, ZF, SF but probably not needed

	template<typename T1, typename T2, typename T3>
	InlineVoid shrd(T1 &val1, const T2 val2, const T3 cnt)
	{
		ASSERT_SIZES(val1, val2);
		val1 = (UNSIGNED(val1) >> cnt) | (val2 << ((sizeof val1 << 3) - cnt));
	} //TODO: CF, OF, ZF, SF, but not needed

	template<typename T>
	InlineVoid bsr(T &val1, const uint32_t val2)
	{
		if (val2)
		{
			val1 = 0x1F - __builtin_clz(val2);
			flags.zf = 0;
		}
		else
		{
			flags.zf = 1;
		}
	}

	/* DF flag is ignored */

	InlineVoid movsb()
	{
		*(int8_t *)edi = *(int8_t *)esi;
		esi += 1;
		edi += 1;
	}
	InlineVoid movsw()
	{
		*(int16_t *)edi = *(int16_t *)esi;
		esi += 2;
		edi += 2;
	}
	InlineVoid movsd()
	{
		*(int32_t *)edi = *(int32_t *)esi;
		esi += 4;
		edi += 4;
	}

	InlineVoid stosb()
	{
		*(int8_t *)edi = al;
		edi += 1;
	}
	InlineVoid stosd()
	{
		*(int32_t *)edi = eax;
		edi += 4;
	}

	InlineVoid scasb()
	{
		const int8_t val = al - *(int8_t *)edi;
		set_ZF_SF_flags(val);
		edi += 1;
	} //TODO: OF, CF, but not needed

	InlineVoid cmpsb()
	{
		cmp(*(int8_t *)esi, *(int8_t *)edi);
		esi += 1;
		edi += 1;
	}

	InlineVoid cpuid()
	{
		//Should not be called
	}

	/* FPU */

	InlineVoid finit()
	{
		fpu.init();
	}

	InlineVoid fnstsw(int16_t &val)
	{
		val = fpu.status.word;
	}

	InlineVoid fstcw(int16_t &val)
	{
		val = fpu.control.word;
	}
	InlineVoid fldcw(int16_t val)
	{
		fpu.control.word = val;
	}

	InlineVoid fldz()
	{
		fpu.push(0.0);
	}
	InlineVoid fld1()
	{
		fpu.push(1.0);
	}

	InlineVoid fchs()
	{
		fpu.st(0) = -fpu.st(0);
	}

	InlineVoid frndint()
	{
		//This game uses "control.rc == 3" which does the same as casting to integer,
		//so make this method NO-OP.
	}

	InlineVoid fcompp()
	{
		fpu.compare(fpu.st(0), fpu.st(1));
		fpu.pop();
		fpu.pop();
	}

	template<typename T>
	InlineVoid fild(const T val)
	{
		static_assert(!is_floating_point<decltype(val)>::value, "FILD requires integer!");
		fpu.push(val);
	}
	template<typename T>
	InlineVoid fld(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FLD requires floating-point!");
		fpu.push(val);
	}
	InlineVoid fld_st(const int32_t num)
	{
		fpu.push(fpu.st(num));
	}

	template<typename T>
	InlineVoid fistp(T &val)
	{
		static_assert(!is_floating_point<GET_TYPE(val)>::value, "FISTP requires integer!");
		val = fpu.st(0);
		fpu.pop();
	}
	template<typename T>
	InlineVoid fstp(T &addr)
	{
		fpu.store(addr);
		fpu.pop();
	}
	InlineVoid fstp_st(const int32_t num)
	{
		fpu.st(num) = fpu.st(0);
		fpu.pop();
	}
	template<typename T>
	InlineVoid fst(T &addr)
	{
		fpu.store(addr);
	}
	InlineVoid fst_st(const int32_t num)
	{
		fpu.st(num) = fpu.st(0);
	}

	InlineVoid fsqrt()
	{
		fpu.st(0) = sqrt(fpu.st(0));
	}
	InlineVoid fcos()
	{
		fpu.st(0) = cos(fpu.st(0));
	}
	InlineVoid fptan()
	{
		fpu.st(0) = tan(fpu.st(0));
		fpu.push(1.0);
	}

	InlineVoid fxch_st(const int32_t num)
	{
		swap(fpu.st(0), fpu.st(num));
	}

	template<typename T>
	InlineVoid fcomp(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FCOMP requires floating-point!");
		fpu.compare(fpu.st(0), val);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fadd(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FADD requires floating-point!");
		fpu.st(0) += val;
	}
	InlineVoid fadd_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) += fpu.st(num2);
	}
	InlineVoid faddp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) += fpu.st(num2);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fsub(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FSUB requires floating-point!");
		fpu.st(0) -= val;
	}
	InlineVoid fsub_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) -= fpu.st(num2);
	}
	InlineVoid fsubp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) -= fpu.st(num2);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fsubr(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FSUBR requires floating-point!");
		fpu.st(0) = val - fpu.st(0);
	}
	InlineVoid fsubrp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) = fpu.st(num2) - fpu.st(num1);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fmul(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FMUL requires floating-point!");
		fpu.st(0) *= val;
	}
	InlineVoid fmul_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) *= fpu.st(num2);
	}
	InlineVoid fmulp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) *= fpu.st(num2);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fdiv(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FDIV requires floating-point!");
		fpu.st(0) /= val;
	}
	InlineVoid fdiv_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) /= fpu.st(num2);
	}
	InlineVoid fdivp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) /= fpu.st(num2);
		fpu.pop();
	}

	template<typename T>
	InlineVoid fdivr(const T val)
	{
		static_assert(is_floating_point<decltype(val)>::value, "FDIVR requires floating-point!");
		fpu.st(0) = val / fpu.st(0);
	}
	InlineVoid fdivrp_st(const int32_t num1, const int32_t num2)
	{
		fpu.st(num1) = fpu.st(num2) / fpu.st(num1);
		fpu.pop();
	}
};

void *main_game_thread, *audio_game_thread;
