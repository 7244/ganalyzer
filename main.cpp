#include <WITCH/WITCH.h>

const uint32_t g_SamplePerSecond = 48000;
typedef sint16_t g_SampleType;
const uint32_t g_TransformSize = 4096;
const uint32_t g_StepSize = g_TransformSize / 2;

#include <WITCH/A/A.h>
#include <WITCH/FS/FS.h>
#include <WITCH/IO/print.h>
#include <WITCH/MATH/MATH.h>

#include <fftw3.h>

void print(const char *format, ...){
  IO_fd_t fd_stdout;
  IO_fd_set(&fd_stdout, FD_ERR);
  va_list argv;
  va_start(argv, format);
  IO_vprint(&fd_stdout, format, argv);
  va_end(argv);
}

#include <vector>
#include <functional>
#include <string>
#include <fstream>

#include "ganalyzer/ganalyzer.h"
struct pile_t{
  ganalyzer_t<g_SampleType> ganalyzer{g_TransformSize};
  std::string InputFileName;
  bool AnalyzeInput = false;

  uint32_t SaveNoteAt = (uint32_t)-1;
  std::string SaveNoteTo;
}pile;

std::vector<g_SampleType> ReadSampleFile(const std::string FileName){
  FS_file_t file_input;
  if(FS_file_open(pile.InputFileName.c_str(), &file_input, O_RDONLY) != 0){
    print("failed to open sample file `%s`\n", pile.InputFileName.c_str());
    PR_exit(0);
  }

  IO_fd_t fd_input;
  FS_file_getfd(&file_input, &fd_input);

  IO_stat_t st;
  IO_fstat(&fd_input, &st);

  IO_off_t file_input_size = IO_stat_GetSizeInBytes(&st);
  if constexpr(sizeof(IO_off_t) > sizeof(uintptr_t)){
    if(file_input_size / sizeof(g_SampleType) > (IO_off_t)(uintptr_t)-1){
      print("sample file is too big\n");
      PR_exit(0);
    }
  }

  if(file_input_size % sizeof(g_SampleType)){
    print("sample file size is not divideable by SampleType\n");
    PR_exit(0);
  }

  uintptr_t SampleCount = file_input_size / sizeof(g_SampleType);

  std::vector<g_SampleType> r;
  r.resize(SampleCount);

  FS_file_read(&file_input, &r[0], file_input_size);

  FS_file_close(&file_input);

  return r;
}

struct ProgramParameter_t{
  std::string name;
  std::function<void(uint32_t, const char **)> func;
};
ProgramParameter_t ProgramParameters[] = {
  {
    "h",
    [](uint32_t argCount, const char **arg){
      print("-h == prints this and exits program.\n");
      print("-i == input file name, <file/to/path>\n");
      print("-a == analyze input\n");
      print("-on == output note, <note index> <file/to/path>\n");
      print("-in == input note, <file/to/path>\n");
      PR_exit(0);
    }
  },
  {
    "i",
    [](uint32_t argCount, const char **arg){
      if(argCount != 1){
        print("parameter -i needs 1 value.\n");
        PR_exit(0);
      }
      pile.InputFileName = arg[0];
    }
  },
  {
    "a",
    [](uint32_t argCount, const char **arg){
      if(argCount != 0){
        print("-a parameter doesnt take any value.\n");
        PR_exit(0);
      }
      pile.AnalyzeInput = true;
    }
  },
  {
    "on",
    [](uint32_t argCount, const char **arg){
      if(argCount != 2){
        print("-on parameter needs to take 2 value.\n");
        PR_exit(0);
      }
      pile.SaveNoteAt = std::atoi(arg[0]);
      pile.SaveNoteTo = std::string(arg[1]);
    }
  },
  {
    "in",
    [](uint32_t argCount, const char **arg){
      if(argCount != 1){
        print("-in parameter needs to take a value.\n");
        PR_exit(0);
      }
      pile.ganalyzer.import_note(std::string(arg[0]));
    }
  }
};
constexpr uint32_t ProgramParameterCount = sizeof(ProgramParameters) / sizeof(ProgramParameters[0]);

void CallParameter(uint32_t ParameterAt, uint32_t iarg, const char **argv){
  uint32_t is = 0;
  for(; is < ProgramParameterCount; is++){
    if(std::string(&argv[ParameterAt][1]) == ProgramParameters[is].name){
      break;
    }
  }
  if(is == ProgramParameterCount){
    print("undefined parameter `%s`. -h for help\n", argv[ParameterAt]);
    PR_exit(0);
  }
  ProgramParameters[is].func(iarg - ParameterAt - 1, &argv[ParameterAt + 1]);
}

int main(int argc, const char **argv){
  if(argc <= 1){
    ProgramParameters[0].func(0, NULL);
    return 1;
  }

  {
    uint32_t ParameterAt = 1;
    for(uint32_t iarg = 1; iarg < argc; iarg++){
      if(argv[iarg][0] == '-'){
        if(ParameterAt == iarg){
          continue;
        }
        CallParameter(ParameterAt, iarg, argv);
        ParameterAt = iarg;
      }
      else{
        if(ParameterAt == iarg){
          print("parameter should have a -\n");
          return 0;
        }
      }
    }
    if(ParameterAt < argc){
      CallParameter(ParameterAt, argc, argv);
    }
  }

  if(pile.AnalyzeInput == true || pile.SaveNoteAt != (uint32_t)-1){
    auto Samples = ReadSampleFile(pile.InputFileName);

    uint32_t TotalLines = Samples.size() / g_TransformSize;

    for(uint32_t y = 0; y < TotalLines; y++){
      pile.ganalyzer.analyze(&Samples[y * g_TransformSize]);

      if(pile.SaveNoteAt == pile.ganalyzer.get_AnalyzeCount()){
        pile.ganalyzer.export_current_note(pile.SaveNoteTo);
      }

      if(pile.AnalyzeInput == true){
        print(
          "continuity %lf size %u i %lu\n",
          pile.ganalyzer.get_Continuity(), pile.ganalyzer.get_NoteInfoCount(), pile.ganalyzer.get_AnalyzeCount()
        );
      }
    }
  }


  return 0;
}
