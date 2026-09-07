#pragma once
/* =====================================================================
   ARGENTUM PLATE — plate reverberation core (C++ / JUCE port)

   This is a line-for-line port of the original AudioWorkletProcessor
   (src/audio/plate-worklet.ts) from the web version of the plugin.
   Every constant, coefficient and signal-flow step below mirrors the
   JS source exactly so the two engines produce (bit-for-bit as close
   as double vs. JS-double arithmetic allows) identical audio.

   Topology: Dattorro (JAES 1997) "figure-of-eight" plate tank, in the
   style of Griesinger / EMT 140.
   ===================================================================== */

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <random>
#include <atomic>

namespace argentum
{

constexpr double kBaseSampleRate = 29761.0;

//======================================================================
/** Power-of-two circular delay line with linear-interpolated fractional
    read, matching the JS DelayLine class exactly. */
class DelayLine
{
public:
    void resize (int maxLen)
    {
        int n = 1;
        while (n < maxLen) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }

    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }

    inline void write (double v) noexcept
    {
        buf[(size_t) w] = (float) v;
        w = (w + 1) & mask;
    }

    inline double read (double d) const noexcept
    {
        const double p = (double) w - d;
        const int i = (int) std::floor (p);
        const double f = p - (double) i;
        const float a = buf[(size_t) (i & mask)];
        const float b = buf[(size_t) ((i + 1) & mask)];
        return a + (b - a) * f;
    }

private:
    std::vector<float> buf;
    int mask = 0;
    int w = 0;
};

/** Two-multiply lattice all-pass: H(z) = (z^-M - g) / (1 - g z^-M) */
static inline double allpass (DelayLine& line, double d, double g, double x) noexcept
{
    const double dl = line.read (d);
    const double v = x + g * dl;
    line.write (v);
    return dl - g * v;
}

//======================================================================
struct PlateMode
{
    const char* name;
    int density;     // 0/1 — extra lattice stage
    double hfMul;
    double lowMid;
    int monoIn;       // 0/1
    double diffTrim;
};

static const PlateMode kModes[5] =
{
    { "CHROME",     0, 1.0,  0.0,  0, 1.0  },
    { "COBALT",     0, 0.62, 0.34, 0, 1.02 },
    { "ALUMINIUM",  1, 1.25, 0.0,  0, 0.97 },
    { "UNOBTANIUM", 1, 1.9,  0.0,  0, 0.94 },
    { "OSMIUM",     0, 0.5,  0.18, 1, 1.0  },
};

//======================================================================
/** Per-block parameter snapshot, read once at the top of processBlock —
    mirrors the k-rate AudioParams of the original AudioWorkletProcessor. */
struct PlateParams
{
    double predelay  = 12.0;   // ms      [0, 250]
    double size      = 1.0;    //         [0.25, 2.5]
    double decay     = 2.4;    // s       [0.2, 30]
    double diffusion = 1.0;    //         [0.2, 1.0]
    double damp      = 6200.0; // Hz      [700, 20000]
    double bassMul   = 1.35;   //         [0.1, 4]
    double crossover = 380.0;  // Hz      [60, 1600]
    double modDepth  = 0.28;   //         [0, 1]
    double modRate   = 0.35;   // Hz      [0.02, 4]
    double lowCut    = 90.0;   // Hz      [20, 1000]
    double highCut   = 12000.0;// Hz      [1200, 20000]
    double width     = 1.0;    //         [0, 2]
    double mix       = 0.32;   //         [0, 1]
    double duck      = 0.0;    //         [0, 1]
    bool   freeze     = false;
    bool   bypass     = false;
    bool   trueStereo = true;
    int    mode       = 0;     // 0..4 -> kModes
};

//======================================================================
class PlateReverbDSP
{
public:
    void prepare (double sampleRateIn)
    {
        sr = sampleRateIn;
        srScale = sr / kBaseSampleRate;
        const double S = srScale;
        const double MAXSIZE = 2.6;

        auto mk = [&] (int n) { DelayLine d; d.resize ((int) std::ceil (n * S * MAXSIZE) + 8); return d; };

        N_id1 = 142; N_id2 = 107; N_id3 = 379; N_id4 = 277;
        N_apm1 = 672; N_delA = 4453; N_apB = 1800; N_delB = 3720;
        N_apm2 = 908; N_delC = 4217; N_apD = 2656; N_delD = 3163;
        N_xL = 1074; N_xR = 1234;

        idL[0] = mk (N_id1); idL[1] = mk (N_id2); idL[2] = mk (N_id3); idL[3] = mk (N_id4);
        idR[0] = mk (N_id1); idR[1] = mk (N_id2); idR[2] = mk (N_id3); idR[3] = mk (N_id4);

        apm1 = mk (N_apm1); delA = mk (N_delA);
        apB  = mk (N_apB);  delB = mk (N_delB);
        apm2 = mk (N_apm2); delC = mk (N_delC);
        apD  = mk (N_apD);  delD = mk (N_delD);
        xLn  = mk (N_xL);   xRn  = mk (N_xR);

        pdL.resize ((int) std::ceil (sr * 0.26) + 8);
        pdR.resize ((int) std::ceil (sr * 0.26) + 8);

        reset();
        smInit = false;
        rng.seed (0xA26E17u);
    }

    void reset()
    {
        for (auto& d : idL) d.clear();
        for (auto& d : idR) d.clear();
        apm1.clear(); delA.clear(); apB.clear(); delB.clear();
        apm2.clear(); delC.clear(); apD.clear(); delD.clear();
        xLn.clear(); xRn.clear(); pdL.clear(); pdR.clear();

        bwL = bwR = dampL = dampR = bassL = bassR = 0.0;
        hcL = hcR = lc1L = lc1R = lc2L = lc2R = 0.0;
        lmL1 = lmL2 = lmR1 = lmR2 = 0.0;
        env = 0.0; duckGain = 1.0;
        lfoPhase = 0.0; rw1 = rw2 = 0.0;
        denorm = 1e-20;
        wetAcc = inAcc = 0.0; mCount = 0;
    }

    /** Processes one block in place. inL/inR/outL/outR may alias.
        Returns the RMS wet and dry levels + current duck gain for metering
        (updated roughly every 512 samples, same cadence as the original). */
    void processBlock (const float* inL, const float* inR,
                        float* outL, float* outR, int nFrames,
                        const PlateParams& p) noexcept
    {
        const double S = srScale;
        const PlateMode& M = kModes[std::clamp (p.mode, 0, 4)];

        const bool bypass = p.bypass;

        const double size = std::clamp (p.size, 0.25, 2.5);
        const double k = S * size;
        const double loopT = (21589.0 * k) / sr;
        const bool freeze = p.freeze;
        const double rt60 = p.decay;
        double decayG = freeze ? 0.99995 : std::pow (10.0, (-0.75 * loopT) / std::max (0.05, rt60));
        decayG = std::min (0.99995, std::max (0.0, decayG));

        const double diff = p.diffusion * M.diffTrim;
        const double idg1 = 0.75 * diff, idg2 = 0.625 * diff;
        const double dd1 = std::min (0.78, 0.7 * diff);
        const double dd2 = std::min (0.5, std::max (0.25, 0.5 * diff));

        const double dampHz = std::min (sr * 0.48, p.damp * M.hfMul);
        const double dampC = freeze ? 0.0 : std::exp ((-2.0 * M_PI * dampHz) / sr);

        const double bassLimit = 0.9994 / std::max (1e-6, decayG * decayG);
        const double bass = freeze ? 1.0 : std::min (p.bassMul, bassLimit);
        const double crossC = std::exp ((-2.0 * M_PI * p.crossover) / sr);

        const double exc = 16.0 * S * p.modDepth * std::min (1.0, size * 1.2);
        const double phInc = (2.0 * M_PI * p.modRate) / sr;

        const double lcG = std::tan ((M_PI * std::min (p.lowCut, sr * 0.45)) / sr);
        const double hcG = std::tan ((M_PI * std::min (p.highCut, sr * 0.45)) / sr);
        const double mix = p.mix;
        const double width = p.width;
        const double duckAmt = p.duck;
        const bool trueStereo = p.trueStereo && ! M.monoIn;

        if (! smInit)
        {
            sm.size = size; sm.decayG = decayG; sm.dampC = dampC; sm.bass = bass;
            sm.cross = crossC; sm.exc = exc; sm.mix = mix; sm.width = width;
            sm.lc = lcG; sm.hc = hcG;
            smInit = true;
        }
        const double a = 1.0 - std::exp (-1.0 / ((0.02 * sr) / std::max (1, nFrames)));
        sm.size   += (size   - sm.size)   * a * 0.6;
        sm.decayG += (decayG - sm.decayG) * a;
        sm.dampC  += (dampC  - sm.dampC)  * a;
        sm.bass   += (bass   - sm.bass)   * a;
        sm.cross  += (crossC - sm.cross)  * a;
        sm.exc    += (exc    - sm.exc)    * a;
        sm.mix    += (mix    - sm.mix)    * a;
        sm.width  += (width  - sm.width)  * a;
        sm.lc     += (lcG    - sm.lc)     * a;
        sm.hc     += (hcG    - sm.hc)     * a;

        const double ks = S * sm.size;
        const double dG = sm.decayG;
        const double dC = sm.dampC;
        const double bM = sm.bass;
        const double xC = sm.cross;
        const double EX = sm.exc;

        const double L_id1 = N_id1 * S, L_id2 = N_id2 * S, L_id3 = N_id3 * S, L_id4 = N_id4 * S;
        const double L_apm1 = N_apm1 * ks, L_delA = N_delA * ks, L_apB = N_apB * ks, L_delB = N_delB * ks;
        const double L_apm2 = N_apm2 * ks, L_delC = N_delC * ks, L_apD = N_apD * ks, L_delD = N_delD * ks;
        const double L_xL = N_xL * ks, L_xR = N_xR * ks;

        auto T = [&] (double n) { return n * ks; };
        const double t1 = T (266),  t2 = T (2974), t3 = T (1913), t4 = T (1996), t5 = T (1990), t6 = T (187),  t7 = T (1066);
        const double u1 = T (353),  u2 = T (3627), u3 = T (1228), u4 = T (2673), u5 = T (2111), u6 = T (335),  u7 = T (121);

        const double pdSamples = std::max (1.0, (p.predelay / 1000.0) * sr);
        const double bwC = 0.9995;
        const double inputGain = freeze ? 0.0 : 1.0;

        const double atkC = std::exp (-1.0 / (0.004 * sr));
        const double relC = std::exp (-1.0 / (0.18 * sr));

        const double lmG = std::tan ((M_PI * 250.0) / sr);
        const double lmR = 1.0 / (2.0 * 1.1);
        const double lmD = 1.0 / (1.0 + 2.0 * lmR * lmG + lmG * lmG);

        std::uniform_real_distribution<double> uni (-1.0, 1.0);

        for (int i = 0; i < nFrames; ++i)
        {
            const double dryL = inL ? inL[i] : 0.0;
            const double dryR = inR ? inR[i] : dryL;

            if (bypass)
            {
                outL[i] = (float) dryL;
                outR[i] = (float) dryR;
                continue;
            }

            const double rect = std::max (std::abs (dryL), std::abs (dryR));
            env = rect > env ? atkC * env + (1.0 - atkC) * rect
                              : relC * env + (1.0 - relC) * rect;
            const double dgt = 1.0 / (1.0 + duckAmt * 8.0 * env);
            duckGain += (dgt - duckGain) * 0.01;

            pdL.write (dryL);
            pdR.write (dryR);
            double pl = pdL.read (pdSamples);
            double pr = pdR.read (pdSamples);
            if (M.monoIn || ! trueStereo)
            {
                const double mm = 0.5 * (pl + pr);
                pl = mm; pr = mm;
            }
            pl *= inputGain; pr *= inputGain;

            bwL = (1.0 - bwC) * bwL + bwC * pl;
            bwR = (1.0 - bwC) * bwR + bwC * pr;

            double aL = allpass (idL[0], L_id1, idg1, bwL);
            aL = allpass (idL[1], L_id2, idg1, aL);
            aL = allpass (idL[2], L_id3, idg2, aL);
            aL = allpass (idL[3], L_id4, idg2, aL);

            double aR;
            if (trueStereo)
            {
                aR = allpass (idR[0], L_id1, idg1, bwR);
                aR = allpass (idR[1], L_id2, idg1, aR);
                aR = allpass (idR[2], L_id3, idg2, aR);
                aR = allpass (idR[3], L_id4, idg2, aR);
            }
            else
            {
                aR = aL;
            }

            lfoPhase += phInc;
            if (lfoPhase > 2.0 * M_PI) lfoPhase -= 2.0 * M_PI;
            const double s1 = std::sin (lfoPhase);
            const double s2 = std::sin (lfoPhase + 2.399);
            rw1 += 0.00004 * (uni (rng) - rw1 * 0.12);
            rw2 += 0.00004 * (uni (rng) - rw2 * 0.12);
            const double m1 = 0.7 * s1 + 0.3 * std::clamp (rw1 * 90.0, -1.0, 1.0);
            const double m2 = 0.7 * s2 + 0.3 * std::clamp (rw2 * 90.0, -1.0, 1.0);

            const double tailL = delB.read (L_delB);
            const double tailR = delD.read (L_delD);
            denorm = -denorm;
            const double dn = denorm;

            double x = aL + dG * tailR + dn;
            x = allpass (apm1, std::max (2.0, L_apm1 + EX * m1), dd1, x);
            delA.write (x);
            double v = delA.read (L_delA);
            dampL = (1.0 - dC) * v + dC * dampL;
            v = dampL;
            bassL = (1.0 - xC) * v + xC * bassL;
            v = v - bassL + bM * bassL;
            if (M.lowMid > 0.0)
            {
                const double hp = (v - (2.0 * lmR + lmG) * lmL1 - lmL2) * lmD;
                const double bp = lmG * hp + lmL1;
                lmL1 = lmG * hp + bp;
                lmL2 = lmG * bp + (lmG * bp + lmL2);
                v += M.lowMid * bp;
            }
            v *= dG;
            v = allpass (apB, L_apB, -dd2, v);
            if (M.density) v = allpass (xLn, L_xL, 0.5, v);
            delB.write (v);

            double y = aR + dG * tailL + dn;
            y = allpass (apm2, std::max (2.0, L_apm2 + EX * m2), dd1, y);
            delC.write (y);
            double w = delC.read (L_delC);
            dampR = (1.0 - dC) * w + dC * dampR;
            w = dampR;
            bassR = (1.0 - xC) * w + xC * bassR;
            w = w - bassR + bM * bassR;
            if (M.lowMid > 0.0)
            {
                const double hp = (w - (2.0 * lmR + lmG) * lmR1 - lmR2) * lmD;
                const double bp = lmG * hp + lmR1;
                lmR1 = lmG * hp + bp;
                lmR2 = lmG * bp + (lmG * bp + lmR2);
                w += M.lowMid * bp;
            }
            w *= dG;
            w = allpass (apD, L_apD, -dd2, w);
            if (M.density) w = allpass (xRn, L_xR, 0.5, w);
            delD.write (w);

            double yl = delC.read (t1) + delC.read (t2) - apD.read (t3) + delD.read (t4)
                      - delA.read (t5) - apB.read (t6) - delB.read (t7);
            double yr = delA.read (u1) + delA.read (u2) - apB.read (u3) + delB.read (u4)
                      - delC.read (u5) - apD.read (u6) - delD.read (u7);
            yl *= 0.6; yr *= 0.6;

            const double GL = sm.lc / (1.0 + sm.lc);
            {
                const double v1 = (yl - lc1L) * GL, p1 = v1 + lc1L;
                lc1L = p1 + v1; yl -= p1;
                const double v2 = (yl - lc2L) * GL, p2 = v2 + lc2L;
                lc2L = p2 + v2; yl -= p2;

                const double v3 = (yr - lc1R) * GL, p3 = v3 + lc1R;
                lc1R = p3 + v3; yr -= p3;
                const double v4 = (yr - lc2R) * GL, p4 = v4 + lc2R;
                lc2R = p4 + v4; yr -= p4;
            }

            const double GH = sm.hc / (1.0 + sm.hc);
            {
                const double vhl = (yl - hcL) * GH;
                const double lpl = vhl + hcL;
                hcL = lpl + vhl;
                yl = lpl;
                const double vhr = (yr - hcR) * GH;
                const double lpr = vhr + hcR;
                hcR = lpr + vhr;
                yr = lpr;
            }

            const double mid = 0.5 * (yl + yr);
            const double side = 0.5 * (yl - yr) * sm.width;
            yl = mid + side;
            yr = mid - side;

            yl *= duckGain;
            yr *= duckGain;
            const double wetG = std::sin ((sm.mix * M_PI) / 2.0);
            const double dryG = std::cos ((sm.mix * M_PI) / 2.0);

            outL[i] = (float) (dryL * dryG + yl * wetG);
            outR[i] = (float) (dryR * dryG + yr * wetG);

            wetAcc += yl * yl + yr * yr;
            inAcc += dryL * dryL + dryR * dryR;
            mCount += 2;
        }

        meterCount += nFrames;
        if (meterCount >= 512)
        {
            meterCount = 0;
            const double n = std::max (1, mCount);
            lastWetRms.store ((float) std::sqrt (wetAcc / n));
            lastInRms.store ((float) std::sqrt (inAcc / n));
            lastDuck.store ((float) duckGain);
            wetAcc = 0.0; inAcc = 0.0; mCount = 0;
        }
    }

    // Meter readouts for the editor (safe to poll from the message thread).
    std::atomic<float> lastWetRms { 0.0f };
    std::atomic<float> lastInRms  { 0.0f };
    std::atomic<float> lastDuck   { 1.0f };

private:
    double sr = 48000.0, srScale = 1.0;

    int N_id1 = 0, N_id2 = 0, N_id3 = 0, N_id4 = 0;
    int N_apm1 = 0, N_delA = 0, N_apB = 0, N_delB = 0;
    int N_apm2 = 0, N_delC = 0, N_apD = 0, N_delD = 0;
    int N_xL = 0, N_xR = 0;

    DelayLine idL[4], idR[4];
    DelayLine apm1, delA, apB, delB;
    DelayLine apm2, delC, apD, delD;
    DelayLine xLn, xRn;
    DelayLine pdL, pdR;

    double bwL = 0, bwR = 0, dampL = 0, dampR = 0, bassL = 0, bassR = 0;
    double lmL1 = 0, lmL2 = 0, lmR1 = 0, lmR2 = 0;
    double hcL = 0, hcR = 0, lc1L = 0, lc1R = 0, lc2L = 0, lc2R = 0;
    double env = 0, duckGain = 1;
    double lfoPhase = 0, rw1 = 0, rw2 = 0;
    double denorm = 1e-20;
    int meterCount = 0;
    double wetAcc = 0, inAcc = 0;
    int mCount = 0;

    struct Smoothed
    {
        double size = 1, decayG = 0.5, dampC = 0, bass = 1, cross = 0, exc = 0,
               mix = 0.3, width = 1, lc = 0, hc = 0;
    } sm;
    bool smInit = false;

    std::mt19937 rng { 0xA26E17u };
};

} // namespace argentum
