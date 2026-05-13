#ifndef PID_H
#define PID_H

// PID settings
#define PID_SET_POINT 0.0f // %
#define PID_K_P 1.0f
#define PID_K_I 0.05f
#define PID_K_D 0.0f // Not used
#define OUT_CLAMP_MIN 0.0f // Percent
#define OUT_CLAMP_MAX 100.0f // Percent

class CPid
{
  public:
    CPid() {}; // Empty constructor
    ~CPid() {}; // Empty destructor

    void Reset() { m_fIacc = 0.0f; m_fLastErrVal = 0.0f; };
    float UpdateValue(const float& fCurVal, const float& fDt = 1.0f);
    float GetValue() { return m_fOutValue; };
    bool IsClamped() { return m_bClamped; };

  private:
    float m_fSetPoint = PID_SET_POINT;
    float m_fKp = PID_K_P;
    float m_fKi = PID_K_I;
    float m_fKd = PID_K_D;
    float m_fIacc = 0.0f;
    float m_fLastErrVal = 0.0f;
    float m_fOutValue = 0.0f;
    bool m_bClamped = false;
};
#endif // PID_H
