template <typename SampleType>
struct ganalyzer_t{

  uint32_t WindowSize;

  fftw_plan fftw_p;
  double *fftw_in;
  fftw_complex *fftw_out;
  f32_t *ft; /* frequency table */

  struct note_t{
    struct info_t{
      f32_t at;
      f32_t power;
      f32_t size;
    };
    std::vector<info_t> infos;
  };
  struct imported_note_t{
    note_t note;
    f32_t SimilarityToCurrent = 0;
  };
  std::vector<imported_note_t> imported_notes;
  note_t current_note;

  #include "Noise.h"
  Noise_t Noise{0};

  static uintptr_t analyze_in(f32_t *ft, Noise_t &Noise, uintptr_t iStep, note_t &note){
    f32_t prev = ft[iStep];

    while(1){
      if(
        ft[iStep] > prev ||
        ft[iStep] <= 0
      ){
        iStep++;
        break;
      }
      prev = ft[iStep];
      if(iStep == 0){
        break;
      }
      iStep--;
    }

    uintptr_t Begin = iStep;

    bool Decreasing = false;
    for(; iStep < g_StepSize; iStep++){
      if(ft[iStep] <= 0){
        break;
      }
      if(Decreasing == false && ft[iStep] < prev){
        Decreasing = true;
      }
      else if(Decreasing == true && ft[iStep] > prev){
        break;
      }
      prev = ft[iStep];
    }

    if(iStep == Begin){
      return iStep;
    }

    uintptr_t End = iStep - 1;

    analyze_in_FindOptimalRange(Noise, ft, Begin, End);

    if(End - Begin + 1 < 3){
      return iStep;
    }

    f32_t StepSum = 0;

    uintptr_t iBiggest;
    f32_t Biggest = -9999;
    for(uintptr_t i = Begin; i <= End; i++){
      StepSum += ft[i];
      if(ft[i] > Biggest){
        iBiggest = i;
        Biggest = ft[i];
      }
    }

    f32_t SideValue = ft[Begin];
    if(SideValue > ft[End]){
      SideValue = ft[End];
    }
    if(SideValue * 1.75 >= ft[iBiggest]){
      return iStep;
    }

    typename note_t::info_t info;
    info.at = iBiggest;
    info.size = (f32_t)End - Begin + 1;

    //print("  this was %u %llf %u\n", iBiggest, StepSum, iStep - Begin);

    note.infos.push_back(info);

    return iStep;
  }

  static void analyze_in_FindOptimalRange(Noise_t &Noise, f32_t *ft, uintptr_t &Start, uintptr_t End){
    while(1){
      f32_t StepSum = 0;
      for(uintptr_t i = Start; i <= End; i++){
        StepSum += ft[i];
      }

      if(StepSum > Noise.Loudest * (End - Start + 1) * 0.25){
        break;
      }

      if(Start == End){
        break;
      }

      if(ft[Start] <= ft[End]){
        Start++;
      }
      else if(ft[End] < ft[Start]){
        End--;
      }
    }
  }

  uintptr_t SubInfoFromNote(
    const note_t::info_t &info,
    note_t &note
  ){
    uintptr_t r = 0;
    for(uintptr_t iInfo = 0; iInfo < note.infos.size();){
      auto &ninfo = note.infos[iInfo];
      //print("  %u %llf %llf\n", iInfo, info.at, ninfo.at);
      if(fabs(info.at - ninfo.at) <= 1.01){
        note.infos.erase(note.infos.begin() + iInfo);
        r++;
        continue;
      }
      iInfo++;
    }
    return r;
  }

  bool DoesIStepHavePotential(uintptr_t iStep, const note_t &note){
    if(ft[iStep] > Noise.Loudest * 0.5){
      return true;
    }
    if(ft[iStep] < Noise.Loudest * 0.25){
      return false;
    }

    if(note.infos.size() < 2){
      return false;
    }

    f32_t at1 = note.infos[note.infos.size() - 1].at;
    f32_t at2 = note.infos[note.infos.size() - 2].at;
    if(fabs((at1 + (at1 - at2)) - (f32_t)iStep) < 1.01){
      return true;
    }

    return false;
  }
  bool _DoesIStepHavePotential0(uintptr_t iStep, const note_t &note){
    return ft[iStep] > Noise.Loudest * 0.5;
  }

  f32_t Continuity;
  uint32_t NoteInfoCount;
  uint32_t AnalyzeCount;

  auto get_ImportedNoteCount(){
    return imported_notes.size();
  }
  auto get_SimilarityOfNote(auto i){
    return imported_notes[i].SimilarityToCurrent;
  }
  auto get_Continuity(){
    return Continuity;
  }
  auto get_NoteInfoCount(){
    return NoteInfoCount;
  }
  auto get_AnalyzeCount(){
    return AnalyzeCount;
  }

  void clear(){
    Continuity = 0;
    NoteInfoCount = 0;
    AnalyzeCount = (decltype(AnalyzeCount))-1;
  }

  void analyze_note(note_t &tnote){
    uintptr_t PastStartHit = 0;
    {
      uintptr_t note_infos_size = current_note.infos.size();
      for(uintptr_t iInfo = 0; iInfo < tnote.infos.size();){
        auto &tinfo = tnote.infos[iInfo];
        SubInfoFromNote(tinfo, current_note);
        iInfo++;
      }
      PastStartHit += note_infos_size - current_note.infos.size();
    }

    current_note = tnote;

    NoteInfoCount = tnote.infos.size();
    if(NoteInfoCount){
      Continuity = (f32_t)PastStartHit / tnote.infos.size();
    }
    else{
      Continuity = 0;
    }

    for(uintptr_t i = 0; i < imported_notes.size(); i++){
      note_t isnote = imported_notes[i].note; /* imported temp note */
      auto oicount = isnote.infos.size(); /* old info count */
      for(uintptr_t iInfo = 0; iInfo < current_note.infos.size(); iInfo++){
        SubInfoFromNote(current_note.infos[iInfo], isnote);
      }
      if(oicount != 0){
        imported_notes[i].SimilarityToCurrent = (f32_t)(oicount - isnote.infos.size()) / oicount;
      }
      else{
        imported_notes[i].SimilarityToCurrent = 1;
      }
    }

    AnalyzeCount++;
  }

  void analyze(SampleType *Samples){
    for(uint32_t x = 0; x < WindowSize; x++){
      double multiplier = 0.5 * (1.0 - cos(2.0 * 3.14 * x / (WindowSize - 1)));
      //double multiplier = 1;
      fftw_in[x] = ((f64_t)Samples[x] / 0x7fff) * multiplier;
    }

    fftw_execute(fftw_p);

    f64_t final[g_StepSize];
    for(uint32_t x = 0; x < g_StepSize; x++){
      f64_t power_in_db = 10 * log(fftw_out[x][0] * fftw_out[x][0] + fftw_out[x][1] * fftw_out[x][1]) / log(10);
      ft[x] = power_in_db + 96;
    }

    Noise.analyze(ft);

    note_t tnote; // transform note or temporar note
    for(uintptr_t iStep = 0; iStep < g_StepSize / 16;){
      if(DoesIStepHavePotential(iStep, tnote)){
        iStep = analyze_in(ft, Noise, iStep, tnote);
      }
      else{
        iStep++;
      }
    }

    analyze_note(tnote);

    #if 0
    for(uintptr_t i = 0; i < tnote.infos.size(); i++){
      print("  left %llf %llf\n", tnote.infos[i].at, tnote.infos[i].size);
    }

    f32_t AllSum = 0;
    for(uintptr_t iStep = 0; iStep < g_StepSize / 16; iStep++){
      AllSum += ft[iStep];
    }
    print("ft sum %llf\n", AllSum);
    #endif
  }

  void export_current_note(std::ofstream &f){
    uint32_t s = current_note.infos.size();
    f.write((const char *)&s, sizeof(s));
    f.write((const char *)&current_note.infos[0], s * sizeof(current_note.infos[0]));
  }
  void import_note(std::string FileName){
    std::ifstream f(FileName);
    if(!f){
      print("note output file is failed to open `%s`\n", FileName.c_str());
      PR_exit(0);
    }

    {
      uint32_t s;
      f.read((char *)&s, sizeof(s));
      note_t note;
      note.infos.resize(s);
      f.read((char *)&note.infos[0], s * sizeof(note.infos[0]));
      imported_notes.push_back({.note = note});
    }
  }
  void iterate_file_notes(std::string FileName, auto cb){
    std::ifstream f(FileName);
    while(f){
      uint32_t s;
      f.read((char *)&s, sizeof(s));
      note_t note;
      note.infos.resize(s);
      f.read((char *)&note.infos[0], s * sizeof(note.infos[0]));
      cb(note);
    }
  }

  ganalyzer_t(uint32_t p_WindowSize){
    clear();
    WindowSize = p_WindowSize;

    fftw_in = (double *)fftw_malloc(WindowSize * sizeof(double));
    if(fftw_in == NULL){
      __abort();
    }
    fftw_out = (fftw_complex *)fftw_malloc(WindowSize * sizeof(fftw_complex));
    if(fftw_out == NULL){
      __abort();
    }
    fftw_p = fftw_plan_dft_r2c_1d(WindowSize, fftw_in, fftw_out, FFTW_ESTIMATE);
    if(fftw_p == NULL){
      __abort();
    }
    ft = (f32_t *)malloc(WindowSize / 2 * sizeof(ft[0]));
    if(ft == NULL){
      __abort();
    }
  }
  ~ganalyzer_t(){
    free(ft);
    fftw_destroy_plan(fftw_p);
    fftw_free(fftw_out);
    fftw_free(fftw_in);
  }
};
