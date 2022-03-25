#ifdef HSK_RTRPF_EXPORT
#define RTRPFLIB __declspec(dllexport)
#else
#define RTRPFLIB __declspec(dllimport)
#endif