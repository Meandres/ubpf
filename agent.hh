#ifndef AGENT_HH
#define AGENT_HH

#include "vm/inc/ubpf.h"
#include <cstdlib>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <mutex>

inline std::vector<uint8_t> read_file(const std::string &filename){
    FILE *file = fopen(filename.c_str(), "rb");
    if(!file){
        return {};
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> buffer((file_size));
    if(fread(buffer.data(), 1, file_size, file) != file_size){
        fclose(file);
        return {};
    }

    fclose(file);
    return buffer;
}

// To register helper functions
// 1.

class Agent{
    public:
    struct ubpf_vm* vm;
    ubpf_jit_fn func;
    std::mutex _lock;

    Agent(const std::string filename, const char* func_name, std::vector<external_function_t> helper_funcs){
        vm = ubpf_create();
        if(!vm){
            throw std::runtime_error("Could not create the ebpf vm.");
        }
        const std::lock_guard<std::mutex> guard(_lock);
        for(int i=0; i<helper_funcs.size(); i++){
            int ret = ubpf_register(vm, i+1, "ebpf_helper", helper_funcs[i]);
            if(ret<0){
                throw std::runtime_error("Could not register the helper function");
            }
        }
        func = load_func(filename, func_name);
    }

    void reload(const std::string filename, const char* func_name){
        const std::lock_guard<std::mutex> guard(_lock);
        ubpf_unload_code(vm);
        func = load_func(filename, func_name);
    }
    private:

    int register_raw_func(int index, void* func){
        external_function_t f = as_external_function_t(func);
        std::string name("ebpf-");
        name+=std::to_string(index);
        int ret = ubpf_register(vm, index, name.c_str(),f);
        return 0;
    }

    ubpf_jit_fn load_func(const std::string filename, const char* func_name){
        char* errmsg = (char*)malloc(100 * sizeof(char));
        const std::lock_guard<std::mutex> guard(_lock);
        std::vector<uint8_t> buffer = read_file(filename);
        if(buffer.empty()){
            throw std::runtime_error("Error when reading the ebpf function.");
        }
        ubpf_load_elf_ex(vm, buffer.data(), buffer.size(), func_name, &errmsg);
        if(errmsg != NULL){
            throw std::runtime_error(errmsg);
        }
        ubpf_jit_fn res = ubpf_compile(vm, &errmsg);
        if(errmsg != NULL){
            throw std::runtime_error(errmsg);
        }
        return res;
    }
};

#endif
