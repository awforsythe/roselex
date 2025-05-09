#pragma once

#define t_stringify(x) #x
#define t_stringify_(x) t_stringify(x)
#define t_lineno t_stringify_(__LINE__)

#define t_assert(cond) \
	if (!(cond)) { \
		return "assertion failed: " #cond " (" t_lineno ")"; \
	}

#define t_begin() \
	int num_tests_run = 0; \
	int num_tests_ok = 0;

#define t_run(test) \
	do { \
		num_tests_run++; \
		const char* result = test(); \
		if (result != nullptr) { \
			printf("[FAILED] " #test ": %s\n", result); \
		} else { \
			num_tests_ok++; \
			printf("[  ok  ] " #test "\n"); \
		} \
	} while (0);

#define t_end() \
	printf("%d test(s) run; %d test(s) passed\n", num_tests_run, num_tests_ok); \
	if (num_tests_run > 0 && num_tests_ok == num_tests_run) { \
		printf("OK\n"); \
		return 0; \
	} else { \
		printf("%d FAILED\n", num_tests_run - num_tests_ok); \
		return 1; \
	}
