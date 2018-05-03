#define TESTS
#include "../Code1.cpp"
#undef TESTS

#include <cstdio>

#if 0
#define ARRAY_COUNT(arr) \
	(sizeof(arr) / sizeof(*arr))
#endif

#define ASSERT(expr) \
	if (!(expr)) \
	{ \
		fprintf(stderr, "%s:%u: assertion `%s` failed.\n", __func__, __LINE__, #expr); \
		test_failed = true; \
		tests_failed = true; \
	}

#define COMPARE(v1,v2) \
	if ((v1) != (v2)) \
	{ \
		fprintf(stderr, "%s:%u: expected `0x%llX`, got `0x%llX`.\n", __func__, __LINE__, (uint64_t)v2, (uint64_t)v1); \
		test_failed = true; \
		tests_failed = true; \
	}

#define COMPARE_FLT(v1,v2) \
	if ((v1) != (v2)) \
	{ \
		fprintf(stderr, "%s:%u: expected `%g`, got `%g`.\n", __func__, __LINE__, v2, v1); \
		test_failed = true; \
		tests_failed = true; \
	}

#define RUN_TEST(func) \
	{ \
		Tests tests; \
		tests.test_##func(); \
		fprintf(stderr, "%s: %s\n", #func, tests.test_failed ? "FAIL" : "PASS"); \
		fflush(stderr); \
	}

static bool tests_failed = false;

struct Tests : public Application
{
	bool test_failed = false;

	Tests()
	{
		ASSERT(flags.unused1 == true);
		ASSERT(flags.unused5 == true);
	}

	void test_registers()
	{
		{
			union
			{
				int64_t val;
				struct
				{
					int32_t lo;
					int32_t hi;
				};
			} const v = {0x0123456789ABCDEF};

			edx_eax = v.val;
			COMPARE(eax, v.lo);
			COMPARE(edx, v.hi);

			//Check optimizations
			edx_eax = edx / 2;
			COMPARE(eax, v.hi / 2);
			COMPARE(edx, 0);

			edx_eax = 0;
			COMPARE(eax, 0);
			COMPARE(edx, 0);
		}

		{
			union
			{
				int32_t val32;
				int16_t val16;
				struct
				{
					int8_t lo;
					int8_t hi;
				};
			} const v = {0x12345678};

			eax = v.val32;
			COMPARE(ax, v.val16);
			COMPARE(ah, v.hi);
			COMPARE(al, v.lo);
			eax = 0;

			ecx = v.val32;
			COMPARE(cx, v.val16);
			COMPARE(ch, v.hi);
			COMPARE(cl, v.lo);
			ecx = 0;

			edx = v.val32;
			COMPARE(dx, v.val16);
			COMPARE(dh, v.hi);
			COMPARE(dl, v.lo);
			edx = 0;

			ebx = v.val32;
			COMPARE(bx, v.val16);
			COMPARE(bh, v.hi);
			COMPARE(bl, v.lo);
			ebx = 0;

			esp = v.val32;
			COMPARE(sp, v.val16);
			esp = 0;

			ebp = v.val32;
			COMPARE(bp, v.val16);
			ebp = 0;

			esi = v.val32;
			COMPARE(si, v.val16);
			esi = 0;

			edi = v.val32;
			COMPARE(di, v.val16);
			edi = 0;
		}
	}
	void test_basic_macros()
	{
		COMPARE(getMSB(0x7FFFFFFF), 0);
		COMPARE(getMSB(0x00000000), 0);
		COMPARE(getMSB(0xFFFFFFFF), 1);
		COMPARE(getMSB(0x80000000), 1);

		COMPARE(getMSB((uint16_t)0x7FFF), 0);
		COMPARE(getMSB((uint16_t)0x0000), 0);
		COMPARE(getMSB((uint16_t)0xFFFF), 1);
		COMPARE(getMSB((uint16_t)0x8000), 1);

		COMPARE(getLSB(0b100), 0);
		COMPARE(getLSB(0b101), 1);

		const int64_t v = 0x0123456789ABCDEF;
		COMPARE(to64i(&v), v);
		COMPARE(to32i(&v), (int32_t)v);
		COMPARE(to16i(&v), (int16_t)v);
		COMPARE(to8i(&v),  (int8_t)v);
	}

	void test_stack()
	{
		{
			const int32_t v = 0x12345678;

			const int32_t tmp_esp = esp;

			push32(v);

			COMPARE(esp, tmp_esp - 4);
			COMPARE(to32i(esp), v);

			pop32(eax);

			COMPARE(esp, tmp_esp);
			COMPARE(eax, v);
		}
		{
			const int32_t tmp_esp = esp;

			push32(esp);

			COMPARE(esp, tmp_esp - 4);
			COMPARE(to32i(esp), tmp_esp);

			pop32(eax);

			COMPARE(esp, tmp_esp);
			COMPARE(eax, esp);
		}

		{
			const int16_t v = 0x1234;

			const int32_t tmp_esp = esp;

			push16(v);

			COMPARE(esp, tmp_esp - 4);
			COMPARE(to16i(esp), v);
			COMPARE(to32i(esp), v);

			pop16(ax);

			COMPARE(esp, tmp_esp);
			COMPARE(ax, v);
		}
	}
	void test_stack_pusha_popa()
	{
		const int32_t tmp_esp = esp;

		eax = 0x10203040;
		ecx = 0x20304050;
		edx = 0x30405060;
		ebx = 0x40506070;
		ebp = 0x50607080;
		esi = 0x60708090;
		edi = 0x708090A0;

		pusha();

		COMPARE(esp, tmp_esp - 32);
		COMPARE(to32i(esp +  0), edi);
		COMPARE(to32i(esp +  4), esi);
		COMPARE(to32i(esp +  8), ebp);
		COMPARE(to32i(esp + 12), esp + 32);
		COMPARE(to32i(esp + 16), ebx);
		COMPARE(to32i(esp + 20), edx);
		COMPARE(to32i(esp + 24), ecx);
		COMPARE(to32i(esp + 28), eax);

		eax = -1;
		ecx = -1;
		edx = -1;
		ebx = -1;
		ebp = -1;
		esi = -1;
		edi = -1;

		popa();

		COMPARE(esp, tmp_esp);
		COMPARE(to32i(esp - 32), edi);
		COMPARE(to32i(esp - 28), esi);
		COMPARE(to32i(esp - 24), ebp);
		COMPARE(to32i(esp - 16), ebx);
		COMPARE(to32i(esp - 12), edx);
		COMPARE(to32i(esp -  8), ecx);
		COMPARE(to32i(esp -  4), eax);
	}

	void test_lahf_sahf()
	{
		{
			ah = 0xFF;
			sahf();

			ah = 0x00;
			lahf();

			const CPU::Flags flags = {(uint16_t)ah};
			ASSERT(flags.cf == true);
			ASSERT(flags.unused1 == true);
			ASSERT(flags.pf == true);
			ASSERT(flags.unused2 == false);
			ASSERT(flags.af == true);
			ASSERT(flags.unused3 == false);
			ASSERT(flags.zf == true);
			ASSERT(flags.of == true);
		}

		{
			ah = 0x00;
			sahf();

			ah = 0xFF;
			lahf();

			const CPU::Flags flags = {(uint16_t)ah};
			ASSERT(flags.cf == false);
			ASSERT(flags.unused1 == true);
			ASSERT(flags.pf == false);
			ASSERT(flags.unused2 == false);
			ASSERT(flags.af == false);
			ASSERT(flags.unused3 == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.of == false);
		}
	}

	void test_inc()
	{
		{
			const int32_t v = 0x12345678;

			eax = v;

			inc(eax);

			COMPARE(eax, v + 1);
			ASSERT(!flags.zf);
			ASSERT(!flags.sf);
		}
		{
			const int32_t v = 0x7FFFFFFF;

			eax = v;

			inc(eax);

			COMPARE(eax, (int32_t)((uint32_t)v + 1));
			ASSERT(!flags.zf);
			ASSERT( flags.sf);
		}
		{
			const int32_t v = 0xFFFFFFFF;

			eax = v;

			inc(eax);

			COMPARE(eax, v + 1);
			ASSERT( flags.zf);
			ASSERT(!flags.sf);
		}

		{
			const int16_t v = 0x1234;
			ax = v;
			inc(ax);
			COMPARE(ax, v + 1);
		}

		{
			const int8_t v = 0x12;
			al = v;
			inc(al);
			COMPARE(al, v + 1);
		}
	}
	void test_dec()
	{
		{
			const int32_t v = 0x12345678;

			eax = v;

			dec(eax);

			COMPARE(eax, v - 1);
			ASSERT(!flags.zf);
			ASSERT(!flags.sf);
		}
		{
			const int32_t v = 0x80000001;

			eax = v;

			dec(eax);

			COMPARE(eax, v - 1);
			ASSERT(!flags.zf);
			ASSERT( flags.sf);
		}
		{
			const int32_t v = 0x00000001;

			eax = v;

			dec(eax);

			COMPARE(eax, v - 1);
			ASSERT( flags.zf);
			ASSERT(!flags.sf);
		}

		{
			const int16_t v = 0x1234;
			ax = v;
			dec(ax);
			COMPARE(ax, v - 1);
		}

		{
			const int8_t v = 0x12;
			al = v;
			dec(al);
			COMPARE(al, v - 1);
		}
	}
	void test_add()
	{
		{
			const int32_t v1 = 0x12;
			const int32_t v2 = 0x34;

			eax = v1;
			ebx = v2;

			add(eax, ebx);

			COMPARE(eax, v1 + v2);
			COMPARE(ebx, v2);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			const int32_t v1 = 0x7FFFFFFF;
			const int32_t v2 = 0x00000001;

			eax = v1;
			ebx = v2;

			add(eax, ebx);

			COMPARE(eax, (int32_t)((uint32_t)v1 + (uint32_t)v2));
			COMPARE(ebx, v2);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == true);
		}
		{
			const int32_t v1 = 0xFFFFFFFF;
			const int32_t v2 = 0x00000001;

			eax = v1;
			ebx = v2;

			add(eax, ebx);

			COMPARE(eax, (int32_t)((uint32_t)v1 + (uint32_t)v2));
			COMPARE(ebx, v2);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			const int32_t v1 = 0xFFFFFFFF;
			const int32_t v2 = 0x00000002;

			eax = v1;
			ebx = v2;

			add(eax, ebx);

			COMPARE(eax, (int32_t)((uint32_t)v1 + (uint32_t)v2));
			COMPARE(ebx, v2);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			const int32_t v1 = 0xFFFFFFFF;
			const int32_t v2 = 0xFFFFFFFF;

			eax = v1;
			ebx = v2;

			add(eax, ebx);

			COMPARE(eax, (int32_t)((uint32_t)v1 + (uint32_t)v2));
			COMPARE(ebx, v2);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}

		{
			const int16_t v1 = 0x1111;
			const int16_t v2 = 0x2222;

			ax = v1;
			bx = v2;

			add(ax, bx);

			COMPARE(ax, v1 + v2);
			COMPARE(bx, v2);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}

		{
			const int8_t v1 = 0x11;
			const int8_t v2 = 0x22;

			al = v1;
			bl = v2;

			add(al, bl);

			COMPARE(al, v1 + v2);
			COMPARE(bl, v2);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
	}
	void test_sub()
	{
		{
			eax = 0;
			ebx = 0;

			sub(eax, ebx);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x10;
			ebx = 0x10;

			sub(eax, ebx);
			COMPARE(eax, 0);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x10;
			ebx = 0x15;

			sub(eax, ebx);
			COMPARE(eax, 0x10 - 0x15);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x15;
			ebx = 0x10;

			sub(eax, ebx);
			COMPARE(eax, 0x15 - 0x10);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x00000000;
			ebx = 0x000002B8;

			sub(eax, ebx);
			COMPARE(eax, 0x00000000 - 0x000002B8);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			al = 0x7F;
			bl = 0x80;

			sub(al, bl);
			COMPARE(al, 0x7F - 0x80);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == true);
		}
		{
			al = 0x15;
			bl = 0x80;

			sub(al, bl);
			COMPARE(al, 0x15 - 0x80);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == true);
		}
		{
			al = 0x80;
			bl = 0x15;

			sub(al, bl);
			COMPARE(al, 0x80 - 0x15);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == true);
		}
		{
			eax = 0x80000000;
			ebx = 0x15000000;

			sub(eax, ebx);
			COMPARE(eax, 0x80000000 - 0x15000000);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == true);
		}
		{
			al = 0xF0;
			bl = 0x15;

			sub(al, bl);
			COMPARE(al, int8_t(0xF0 - 0x15));
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			al = 0xFF;
			bl = 0x5F;

			sub(al, bl);
			COMPARE(al, int8_t(0xFF - 0x5F));
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
	}

	void test_cmp()
	{
		{
			eax = 0;
			ebx = 0;

			cmp(eax, ebx);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x10;
			ebx = 0x10;

			cmp(eax, ebx);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x10;
			ebx = 0x15;

			cmp(eax, ebx);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x15;
			ebx = 0x10;

			cmp(eax, ebx);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
		}
		{
			eax = 0x00000000;
			ebx = 0x000002B8;

			cmp(eax, ebx);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			al = 0x7F;
			bl = 0x80;

			cmp(al, bl);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == true);
		}
		{
			al = 0x15;
			bl = 0x80;

			cmp(al, bl);
			ASSERT(flags.cf == true);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == true);
		}
		{
			al = 0x80;
			bl = 0x15;

			cmp(al, bl);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == true);
		}
		{
			eax = 0x80000000;
			ebx = 0x15000000;

			cmp(eax, ebx);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == true);
		}
		{
			al = 0xF0;
			bl = 0x15;

			cmp(al, bl);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
		{
			al = 0xFF;
			bl = 0x5F;

			cmp(al, bl);
			ASSERT(flags.cf == false);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
		}
	}
	void test_test()
	{
		{
			eax = 0x00;
			ebx = 0x10;

			test(eax, ebx);
			ASSERT(flags.zf == true);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == false);
		}
		{
			eax = 0x10;
			ebx = 0x10;

			test(eax, ebx);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == false);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == false);
		}
		{
			eax = 0x80000000;
			ebx = 0xF0000000;

			test(eax, ebx);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == false);
		}
		{
			ax = 0x8000;
			bx = 0xF000;

			test(ax, bx);
			ASSERT(flags.zf == false);
			ASSERT(flags.sf == true);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == false);
		}
	}

	void test_shr()
	{
		{
			const int32_t v1 = 0b11000000000000000000000000001111;

			eax = v1;

			shr(eax, 3);

			COMPARE(eax, (uint32_t)v1 >> 3);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0b11000000000000000000000000001110;

			eax = v1;

			shr(eax, 3);

			COMPARE(eax, (uint32_t)v1 >> 3);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0b11000000000000000000000000001110;

			eax = v1;

			shr(eax, 1);

			COMPARE(eax, (uint32_t)v1 >> 1);
			ASSERT(flags.of == true);
			ASSERT(flags.cf == false);
		}
	}
	void test_sar()
	{
		{
			const int32_t v1 = 0b11000000000000000000000000001111;

			eax = v1;

			sar(eax, 3);

			COMPARE(eax, v1 >> 3);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0b11000000000000000000000000001110;

			eax = v1;

			sar(eax, 3);

			COMPARE(eax, v1 >> 3);
			ASSERT(flags.of == false);
			ASSERT(flags.cf == true);
		}
	}
	void test_shl()
	{
		{
			const int32_t v1 = 0b11000000000000000000000000001111;

			eax = v1;

			shl(eax, 3);

			COMPARE(eax, (uint32_t)v1 << 3);
			ASSERT(flags.cf == false);
			ASSERT(flags.of == false);
		}
		{
			const int32_t v1 = 0b00000000000000000000000000001111;

			eax = v1;

			shl(eax, 3);

			COMPARE(eax, (uint32_t)v1 << 3);
			ASSERT(flags.cf == false);
			ASSERT(flags.of == false);
		}
		{
			const int32_t v1 = 0b10000000000000000000000000001111;

			eax = v1;

			shl(eax, 1);

			COMPARE(eax, (uint32_t)v1 << 1);
			ASSERT(flags.cf == true);
			ASSERT(flags.of == true);
		}
	}
	void test_rol()
	{
		{
			const int32_t v1 = 0b01000000000000000000000000001111;
			const int32_t v2 = 0b00000000000000000000000001111010;

			eax = v1;

			rol(eax, 3);

			COMPARE(eax, v2);
		}
		{
			const int32_t v1 = 0b11000000000000000000000000001110;
			const int32_t v2 = 0b00000000000000000000000001110110;

			eax = v1;

			rol(eax, 3);

			COMPARE(eax, v2);
		}

		{
			const int16_t v1 = 0b0100000000001111;
			const int16_t v2 = 0b0000000001111010;

			ax = v1;

			rol(ax, 3);

			COMPARE(ax, v2);
		}
	}
	void test_ror()
	{
		{
			const int32_t v1 = 0b01000000000000000000000000001111;
			const int32_t v2 = 0b11101000000000000000000000000001;

			eax = v1;

			ror(eax, 3);

			COMPARE(eax, v2);
		}
		{
			const int32_t v1 = 0b11000000000000000000000000001110;
			const int32_t v2 = 0b11011000000000000000000000000001;

			eax = v1;

			ror(eax, 3);

			COMPARE(eax, v2);
		}

		{
			const int16_t v1 = 0b0100000000001111;
			const int16_t v2 = 0b1110100000000001;

			ax = v1;

			ror(ax, 3);

			COMPARE(ax, v2);
		}
	}

	void test_div32()
	{
		{
			const uint64_t v = 0x0123456789ABCDEF;

			edx_eax = v;

			div32(0x10000000);

			COMPARE(eax, 0x12345678);
			COMPARE(edx, 0x09ABCDEF);
		}
		{
			const uint64_t v = 2500;

			edx_eax = v;

			div32(2);

			COMPARE(eax, v / 2);
			COMPARE(edx, v % 2);
		}
		{
			const uint64_t v = 2501;

			edx_eax = v;

			div32(2);

			COMPARE(eax, v / 2);
			COMPARE(edx, v % 2);
		}
	}
	void test_idiv32()
	{
		{
			const int64_t v = 0x0123456789ABCDEF;

			edx_eax = v;

			idiv32(0x10000000);

			COMPARE(eax, 0x12345678);
			COMPARE(edx, 0x09ABCDEF);
		}
		{
			const int64_t v = -2500;

			edx_eax = v;

			idiv32(2);

			COMPARE(eax, v / 2);
			COMPARE(edx, v % 2);
		}
		{
			const int64_t v = -2501;

			edx_eax = v;

			idiv32(2);

			COMPARE(eax, v / 2);
			COMPARE(edx, v % 2);
		}
	}
	void test_mul32()
	{
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x100;

			eax = v1;

			mul32(v2);

			COMPARE(edx_eax, (int64_t)v1 * (int64_t)v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x2;

			eax = v1;

			mul32(v2);

			COMPARE(edx_eax, v1 * v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const uint32_t v1 = 0x82345678;
			const uint32_t v2 = 0x2;

			eax = v1;

			mul32(v2);

			COMPARE(UNSIGNED(edx_eax), (uint64_t)v1 * (uint64_t)v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
	}
	void test_imul32_1()
	{
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x100;

			eax = v1;

			imul32(v2);

			COMPARE(edx_eax, (int64_t)v1 * (int64_t)v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x2;

			eax = v1;

			imul32(v2);

			COMPARE(edx_eax, v1 * v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2;

			eax = v1;

			imul32(v2);

			COMPARE(edx_eax, v1 * v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2000000;

			eax = v1;

			imul32(v2);

			COMPARE(edx_eax, (int64_t)v1 * (int64_t)v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
	}
	void test_imul32_2()
	{
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x10;

			eax = v1;

			imul32(eax, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x2;

			eax = v1;

			imul32(eax, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2;

			eax = v1;

			imul32(eax, v2);

			COMPARE(eax, v1 * v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2000000;

			eax = v1;

			imul32(eax, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
	}
	void test_imul32_3()
	{
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x10;

			imul32(eax, v1, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
		{
			const int32_t v1 = 0x12345678;
			const int32_t v2 = 0x2;

			imul32(eax, v1, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2;

			imul32(eax, v1, v2);

			COMPARE(eax, v1 * v2);
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
		{
			const int32_t v1 = -2500;
			const int32_t v2 = 2000000;

			imul32(eax, v1, v2);

			COMPARE(eax, (int32_t)((int64_t)v1 * (int64_t)v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
	}
	void test_imul16_2()
	{
		{
			const int16_t v1 = 0x1234;
			const int16_t v2 = 0x10;

			ax = v1;

			imul16(ax, v2);

			COMPARE(ax, (int16_t)(v1 * v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == true);
		}
		{
			const int16_t v1 = 0x1234;
			const int16_t v2 = 0x2;

			ax = v1;

			imul16(ax, v2);

			COMPARE(ax, (int16_t)(v1 * v2));
//			COMPARE(flags.cf, flags.of);
//			ASSERT(flags.cf == false);
		}
	}

	void test_neg()
	{
		eax = 10;
		neg(eax);
		COMPARE(eax, -10);
//		ASSERT(flags.cf == true);

		eax = 0;
		neg(eax);
		COMPARE(eax, 0);
//		ASSERT(flags.cf == false);
	}

	void test_bsr()
	{
		bsr(eax, 0);
		ASSERT(flags.zf == true);

		bsr(eax, 1);
		COMPARE(eax, 0);
		ASSERT(flags.zf == false);

		bsr(eax, 2);
		COMPARE(eax, 1);
		ASSERT(flags.zf == false);

		bsr(eax, 3);
		COMPARE(eax, 1);
		ASSERT(flags.zf == false);

		bsr(eax, 4);
		COMPARE(eax, 2);
		ASSERT(flags.zf == false);

		bsr(eax, 15);
		COMPARE(eax, 3);
		ASSERT(flags.zf == false);

		bsr(eax, 16);
		COMPARE(eax, 4);
		ASSERT(flags.zf == false);

		bsr(eax, 17);
		COMPARE(eax, 4);
		ASSERT(flags.zf == false);

		bsr(eax, 31);
		COMPARE(eax, 4);
		ASSERT(flags.zf == false);

		bsr(eax, 32);
		COMPARE(eax, 5);
		ASSERT(flags.zf == false);

		bsr(eax, 32767);
		COMPARE(eax, 14);
		ASSERT(flags.zf == false);

		bsr(eax, 32768);
		COMPARE(eax, 15);
		ASSERT(flags.zf == false);

		bsr(eax, 65535);
		COMPARE(eax, 15);
		ASSERT(flags.zf == false);

		bsr(eax, 65536);
		COMPARE(eax, 16);
		ASSERT(flags.zf == false);

		bsr(eax, 8388608);
		COMPARE(eax, 23);
		ASSERT(flags.zf == false);

		bsr(eax, 2147483647);
		COMPARE(eax, 30);
		ASSERT(flags.zf == false);

		bsr(eax, 1210122529);
		COMPARE(eax, 30);
		ASSERT(flags.zf == false);

		bsr(eax, 2147483648);
		COMPARE(eax, 31);
		ASSERT(flags.zf == false);
	}

	void test_movsb()
	{
		char dataSrc[] = {
			0x10,
			0x20,
			0x05
		};
		char data[3] = {0};

		esi = (int32_t)dataSrc;
		edi = (int32_t)data;

		movsb();
		movsb();
		movsb();

		COMPARE(esi, (int32_t)dataSrc + 3);
		COMPARE(edi, (int32_t)data + 3);
		COMPARE(dataSrc[0], data[0]);
		COMPARE(dataSrc[1], data[1]);
		COMPARE(dataSrc[2], data[2]);
	}
	void test_movsw()
	{
		int16_t dataSrc[] = {
			0x10,
			0x20,
			0x05
		};
		int16_t data[3] = {0};

		esi = (int32_t)dataSrc;
		edi = (int32_t)data;

		movsw();
		movsw();
		movsw();

		COMPARE(esi, (int32_t)(dataSrc + 3));
		COMPARE(edi, (int32_t)(data + 3));
		COMPARE(dataSrc[0], data[0]);
		COMPARE(dataSrc[1], data[1]);
		COMPARE(dataSrc[2], data[2]);
	}
	void test_movsd()
	{
		int32_t dataSrc[] = {
			0x10,
			0x20,
			0x05
		};
		int32_t data[3] = {0};

		esi = (int32_t)dataSrc;
		edi = (int32_t)data;

		movsd();
		movsd();
		movsd();

		COMPARE(esi, (int32_t)(dataSrc + 3));
		COMPARE(edi, (int32_t)(data + 3));
		COMPARE(dataSrc[0], data[0]);
		COMPARE(dataSrc[1], data[1]);
		COMPARE(dataSrc[2], data[2]);
	}
	void test_scasb()
	{
		char data[] = {
			0x10,
			0x20,
			0x05
		};

		al = 0x10;
		edi = (int32_t)data;

		scasb();
		COMPARE(edi, (int32_t)data + 1);
		ASSERT(flags.zf == true);
		ASSERT(flags.sf == false);
		COMPARE(al, 0x10);
		COMPARE(esi, 0);

		scasb();
		COMPARE(edi, (int32_t)data + 2);
		ASSERT(flags.zf == false);
		ASSERT(flags.sf == true);
		COMPARE(al, 0x10);
		COMPARE(esi, 0);

		scasb();
		COMPARE(edi, (int32_t)data + 3);
		ASSERT(flags.zf == false);
		ASSERT(flags.sf == false);
		COMPARE(al, 0x10);
		COMPARE(esi, 0);
	}
	void test_cmpsb()
	{
		char data_src[] = {
			0x10,
			0x10,
			0x10
		};
		char data[] = {
			0x10,
			0x20,
			0x05
		};

		esi = (int32_t)data_src;
		edi = (int32_t)data;

		cmpsb();
		COMPARE(esi, (int32_t)data_src + 1);
		COMPARE(edi, (int32_t)data + 1);
		ASSERT(flags.zf == true);
		ASSERT(flags.sf == false);

		cmpsb();
		COMPARE(esi, (int32_t)data_src + 2);
		COMPARE(edi, (int32_t)data + 2);
		ASSERT(flags.zf == false);
		ASSERT(flags.sf == true);

		cmpsb();
		COMPARE(esi, (int32_t)data_src + 3);
		COMPARE(edi, (int32_t)data + 3);
		ASSERT(flags.zf == false);
		ASSERT(flags.sf == false);
	}

	void test_fpu_status()
	{
		{
			ax = 0xFFFF;
			finit();
			fnstsw(ax);
			sahf();
			ASSERT(flags.cf == false);
			ASSERT(flags.pf == false);
			ASSERT(flags.zf == false);
		}
		{
			ax = 0x0000;
			finit();
			fpu.status.c0 = true;
			fpu.status.c2 = true;
			fpu.status.c3 = true;
			fnstsw(ax);
			sahf();
			ASSERT(flags.cf == true);
			ASSERT(flags.pf == true);
			ASSERT(flags.zf == true);
		}
		{
			ax = 0x0000;
			finit();
			fpu.status.c0 = true;
			fnstsw(ax);
			sahf();
			ASSERT(flags.cf == true);
			ASSERT(flags.pf == false);
			ASSERT(flags.zf == false);
		}
		{
			ax = 0x0000;
			finit();
			fpu.status.c2 = true;
			fnstsw(ax);
			sahf();
			ASSERT(flags.cf == false);
			ASSERT(flags.pf == true);
			ASSERT(flags.zf == false);
		}
		{
			ax = 0x0000;
			finit();
			fpu.status.c3 = true;
			fnstsw(ax);
			sahf();
			ASSERT(flags.cf == false);
			ASSERT(flags.pf == false);
			ASSERT(flags.zf == true);
		}
	}
	void test_fpu_store()
	{
		{
			double a;

			finit();
			fld(12.3);
			fst(to64i(&a));

			COMPARE_FLT(a, 12.3);
		}
		{
			float a;

			finit();
			fld(12.3f);
			fst(to32i(&a));

			COMPARE_FLT(a, 12.3f);
		}
	}
	void test_fpu_get_from_addr()
	{
		{
			const double val = 15.8;
			double a;

			finit();
			fld(get64f(&val));
			fst(to64i(&a));

			COMPARE_FLT(a, val);
		}
		{
			const float val = 15.8f;
			float a;

			finit();
			fld(get32f(&val));
			fst(to32i(&a));

			COMPARE_FLT(a, val);
		}
	}
	void test_fpu_mul()
	{
		{
			float f = 0.0f;
			finit();
			fild(15);
			fmul(2.5f);
			fst(to32i(&f));
			COMPARE_FLT(f, 37.5f);
		}
		{
			double f = 0.0;
			finit();
			fild(15);
			fmul(2.5);
			fst(to64i(&f));
			COMPARE_FLT(f, 37.5);
		}
		{
			double f = 0.0;
			finit();
			fild(15);
			fld(2.5);
			fmul_st(0, 1);
			fstp(to64i(&f));
			COMPARE_FLT(f, 37.5);
			fst(to64i(&f));
			COMPARE_FLT(f, 15.0);
		}
	}
	void test_fpu_xchg()
	{
		double f = 0.0;
		finit();
		fld(4.5);
		fld(8.3);
		fld(3.7);

		fst(to64i(&f));
		COMPARE_FLT(f, 3.7);
		fst(to64i(&f));
		COMPARE_FLT(f, 3.7);

		fxch_st(2);

		fst(to64i(&f));
		COMPARE_FLT(f, 4.5);

		fxch_st(1);

		fst(to64i(&f));
		COMPARE_FLT(f, 8.3);
	}
	void test_fpu_chs()
	{
		{
			double f;
			finit();
			fld(10.5);
			fchs();
			fst(to64i(&f));
			COMPARE_FLT(f, -10.5);
		}
		{
			double f;
			finit();
			fld(-10.5);
			fchs();
			fst(to64i(&f));
			COMPARE_FLT(f, 10.5);
		}
	}
};

/**/

int main()
{
	//TODO: sbb, adc, shrd, xor, or, and; CF, OF flags in inc, dec

	RUN_TEST(registers);
	RUN_TEST(basic_macros);

	RUN_TEST(stack);
	RUN_TEST(stack_pusha_popa);

	RUN_TEST(lahf_sahf);

	RUN_TEST(inc);
	RUN_TEST(dec);
	RUN_TEST(add);
	RUN_TEST(sub);

	RUN_TEST(cmp);
	RUN_TEST(test);

	RUN_TEST(shr);
	RUN_TEST(sar);
	RUN_TEST(shl);
	RUN_TEST(rol);
	RUN_TEST(ror);

	RUN_TEST(div32);
	RUN_TEST(idiv32);
	RUN_TEST(mul32);
	RUN_TEST(imul32_1);
	RUN_TEST(imul32_2);
	RUN_TEST(imul32_3);
	RUN_TEST(imul16_2);

	RUN_TEST(neg);

	RUN_TEST(bsr);

	RUN_TEST(movsb);
	RUN_TEST(movsw);
	RUN_TEST(movsd);
	RUN_TEST(scasb);
	RUN_TEST(cmpsb);

	RUN_TEST(fpu_status);
	RUN_TEST(fpu_store);
	RUN_TEST(fpu_get_from_addr);
	RUN_TEST(fpu_mul);
	RUN_TEST(fpu_xchg);
	RUN_TEST(fpu_chs);

	return tests_failed;
}
