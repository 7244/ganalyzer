struct Noise_t{
  constexpr static uintptr_t size = 128;
  constexpr static uintptr_t StepsPerNoise = g_StepSize / size;
  f32_t f[size];

  f32_t OldLoudest = 0;
  f32_t Loudest;

  Noise_t() = default;
  f32_t smooth(uintptr_t iStep){
    if(iStep < StepsPerNoise / 2){
      return f[0];
    }
    if(iStep > g_StepSize - StepsPerNoise + StepsPerNoise / 2){
      return f[size - 1];
    }

    if(iStep % StepsPerNoise < StepsPerNoise / 2){
      auto m = iStep % StepsPerNoise;
      iStep -= m;
      iStep -= StepsPerNoise - m;
    }

    f32_t s = (f32_t)StepsPerNoise - iStep % StepsPerNoise;
    s = (s + StepsPerNoise / 2) / StepsPerNoise;

    f32_t v0 = f[iStep / StepsPerNoise];
    f32_t v1 = f[iStep / StepsPerNoise + 1];
    f32_t v = std::lerp(v0, v1, s);

    return v;
  }
  f32_t &operator [](uintptr_t iStep){
    return f[iStep / StepsPerNoise];
  }
  void SetAll(f32_t p){
    for(uintptr_t i = 0; i < size; i++){
      f[i] = p;
    }
  }
  Noise_t(f32_t p){
    SetAll(p);
  }

  void AddAverageToNoise(f32_t *final, uintptr_t iStep, uintptr_t iStepTo){
    uintptr_t oiStep = iStep;

    f32_t s = 0;
    for(; iStep < iStepTo; iStep++){
      s += final[iStep];
    }
    s /= iStepTo - oiStep;
    s *= 0.75;

    iStep = oiStep;
    while(1){
      if(s > (*this)[iStep]){
        (*this)[iStep] = s;
      }

      iStep += StepsPerNoise - iStep % StepsPerNoise;
      if(iStep >= iStepTo){
        break;
      }
    }
  }

  constexpr static uint32_t fcount = 8;
  f32_t _f[fcount][size];
  uintptr_t i_f = 0;
  void analyze(f32_t *final){
    for(uintptr_t iNoise = 0; iNoise < size; iNoise++){
      f32_t m = 99999;
      for(uintptr_t iStep = 0; iStep < StepsPerNoise; iStep += 2){
        auto p = &final[iNoise * StepsPerNoise + iStep];
        auto t = p[0] + p[1];
        if(t < m){
          m = t;
        }
      }
      _f[i_f % fcount][iNoise] = m / 2;
    }

    for(uintptr_t i = i_f + 1; i < fcount; i++){
      for(uintptr_t iSize = 0; iSize < size; iSize++){
        _f[i][iSize] = _f[i_f][iSize];
      }
    }
    i_f++;

    for(uintptr_t iSize = 0; iSize < size; iSize++){
      //f[iSize] = (_f[0][iSize] + _f[1][iSize] + _f[2][iSize] + _f[3][iSize]) / 4;
      f[iSize] = 9999;
      for(uintptr_t i = 0; i < fcount; i++){
        f[iSize] = std::min(f[iSize], _f[i][iSize]);
      }
    }

    Loudest = 0;
    for(uintptr_t i = 0; i < g_StepSize; i++){
      final[i] = std::max(final[i] - smooth(i), (f32_t)0);
      if(final[i] > Loudest){
        Loudest = final[i];
      }
    }
    Loudest = std::max(Loudest, (f32_t)(OldLoudest * 0.5));
    OldLoudest = Loudest;
  }
  void _analyze3(f32_t *final){
    for(uintptr_t iNoise = 0; iNoise < size; iNoise++){
      f32_t m = 99999;
      for(uintptr_t iStep = 0; iStep < StepsPerNoise; iStep += 2){
        auto p = &final[iNoise * StepsPerNoise + iStep];
        auto t = p[0] + p[1];
        if(t < m){
          m = t;
        }
      }
      f[iNoise] = m / 2;
    }

    Loudest = 0;
    for(uintptr_t i = 0; i < g_StepSize; i++){
      final[i] = std::max(final[i] - (*this)[i], (f32_t)0);
      if(final[i] > Loudest){
        Loudest = final[i];
      }
    }
    Loudest = std::max(Loudest, (f32_t)(OldLoudest * 0.5));
    OldLoudest = Loudest;
  }
  void _analyze2(f32_t *final){
    for(uintptr_t iNoise = 0; iNoise < size; iNoise++){
      f32_t m = 99999;
      for(uintptr_t iStep = 0; iStep < StepsPerNoise; iStep += 2){
        auto p = &final[iNoise * StepsPerNoise + iStep];
        auto t = p[0] + p[1];
        if(t < m){
          m = t;
        }
      }
      f[iNoise] = m / 2;
    }

    Loudest = 0;
    for(uintptr_t i = 0; i < g_StepSize; i++){
      final[i] = std::max(final[i] - (*this)[i], (f32_t)0);
      if(final[i] > Loudest){
        Loudest = final[i];
      }
    }
    Loudest = std::max(Loudest, (f32_t)(OldLoudest * 0.5));
    OldLoudest = Loudest;

    SetAll(0);

    for(uintptr_t iStep = 0; iStep != g_StepSize;){
      note_t note;

      uintptr_t r = analyze_in(*this, final, iStep, note);

      if(note.infos.size() == 0){
        AddAverageToNoise(final, iStep, r);
      }
      iStep = r;

      note.infos.clear();
    }

    Loudest = 0;
    for(uintptr_t i = 0; i < g_StepSize; i++){
      final[i] = std::max(final[i] - (*this)[i], (f32_t)0);
      if(final[i] > Loudest){
        Loudest = final[i];
      }
    }
    Loudest = std::max(Loudest, (f32_t)(OldLoudest * 0.5));
    OldLoudest = Loudest;
  }
  void _analyze1(f32_t *final){
    for(uintptr_t iNoise = 0; iNoise < size; iNoise++){
      f32_t m = 99999;
      for(uintptr_t iStep = 0; iStep < StepsPerNoise; iStep++){
        f32_t t = final[iNoise * StepsPerNoise + iStep];
        if(t < m){
          m = t;
        }
      }
      f[iNoise] = m;
    }
  }
  void _analyze0(f32_t *final){
    for(uintptr_t iNoise = 0; iNoise < size; iNoise++){
      f32_t Average = 0;
      for(uintptr_t iStep = 0; iStep < StepsPerNoise; iStep++){
        Average += final[iNoise * StepsPerNoise + iStep];
      }
      Average /= StepsPerNoise;
      f[iNoise] = Average;
    }
  }
};
