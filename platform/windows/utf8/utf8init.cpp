extern "C" {
    unsigned __stdcall GetConsoleCP() noexcept;
    unsigned __stdcall GetConsoleOutputCP() noexcept;
    int __stdcall SetConsoleCP(unsigned) noexcept;
    int __stdcall SetConsoleOutputCP(unsigned) noexcept;
}

constexpr int CP_UTF8 = 65001;

struct __utf8init_t {
    unsigned saved_input_cp;
    unsigned saved_output_cp;

    __utf8init_t() noexcept {
        saved_input_cp = GetConsoleCP();
        saved_output_cp = GetConsoleOutputCP();
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    ~__utf8init_t() noexcept {
        SetConsoleCP(saved_input_cp);
        SetConsoleOutputCP(saved_output_cp);
    }
} __utf8init;
