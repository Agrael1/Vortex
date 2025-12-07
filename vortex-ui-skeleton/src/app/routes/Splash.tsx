import { motion } from 'framer-motion';
import { useCallback, useEffect, useMemo, useRef, useState, type ChangeEvent } from 'react';
import { useNavigate } from 'react-router-dom';
import { engine, type LastProject } from '@/app/services/ipc/cefBridge';
import { useProjectCommands } from '@state/hooks/useProjectCommands';
import {
  SPLASH_AUTO_CONTINUE_KEY,
  SPLASH_AUTO_DELAY_PRESETS,
  SPLASH_FALLBACK_DELAY_PRESETS,
  buildDelayOptions,
  clampAutoDelay,
  clampFallbackDelay,
  formatDelayLabel,
  readAutoDelayPreference,
  readFallbackDelayPreference,
  resolveFallbackDelayForContext,
  readSplashStickyPreference,
  writeAutoDelayPreference,
  writeFallbackDelayPreference,
  writeSplashStickyPreference,
} from '@/app/constants/preferences';

export function Splash() {
  const nav = useNavigate();
  const { loadProject } = useProjectCommands();
  const [lastProject, setLastProject] = useState<LastProject | null>(() => engine.getLastProject());
  const [status, setStatus] = useState(() => (lastProject ? `Welcome back, ${lastProject.name}` : 'Initializing workspace…'));
  const [hint, setHint] = useState<string | null>(() => lastProject?.path ?? null);
  const [error, setError] = useState<string | null>(null);
  const [isContinuing, setIsContinuing] = useState(false);
  const [introReady, setIntroReady] = useState(() => {
    if (typeof window === 'undefined') return true;
    return Boolean((window as unknown as { __VortexIntroDone?: boolean }).__VortexIntroDone);
  });
  const [autoContinue, setAutoContinue] = useState(() => {
    if (typeof window === 'undefined') return false;
    return window.localStorage.getItem(SPLASH_AUTO_CONTINUE_KEY) === 'true';
  });
  const [autoDelay, setAutoDelay] = useState(() => readAutoDelayPreference());
  const [fallbackDelay, setFallbackDelay] = useState(() => readFallbackDelayPreference());
  const [stickySplash, setStickySplash] = useState(() => readSplashStickyPreference());
  const [autoCountdown, setAutoCountdown] = useState<number | null>(null);
  const fallbackTimerRef = useRef<number | null>(null);
  const autoTimerRef = useRef<number | null>(null);
  const countdownIntervalRef = useRef<number | null>(null);
  const [showTiming, setShowTiming] = useState(false);
  const autoDelayOptions = useMemo(() => buildDelayOptions(SPLASH_AUTO_DELAY_PRESETS, autoDelay), [autoDelay]);
  const fallbackDelayOptions = useMemo(() => buildDelayOptions(SPLASH_FALLBACK_DELAY_PRESETS, fallbackDelay), [fallbackDelay]);
  const timingSummary = useMemo(
    () => `Auto ${formatDelayLabel(autoDelay)} / Hub ${formatDelayLabel(fallbackDelay)}`,
    [autoDelay, fallbackDelay],
  );

  const clearFallback = useCallback(() => {
    if (fallbackTimerRef.current) {
      window.clearTimeout(fallbackTimerRef.current);
      fallbackTimerRef.current = null;
    }
  }, []);

  const clearAutoTimer = useCallback(() => {
    if (autoTimerRef.current) {
      window.clearTimeout(autoTimerRef.current);
      autoTimerRef.current = null;
    }
    if (countdownIntervalRef.current) {
      window.clearInterval(countdownIntervalRef.current);
      countdownIntervalRef.current = null;
    }
    setAutoCountdown(null);
  }, []);

  const goToHub = useCallback(() => {
    clearFallback();
    nav('/hub', { replace: true });
  }, [clearFallback, nav]);

  const scheduleHubRedirect = useCallback(
    (delay: number) => {
      clearFallback();
      fallbackTimerRef.current = window.setTimeout(goToHub, delay);
    },
    [clearFallback, goToHub],
  );

  const handleAutoDelayChange = useCallback((event: ChangeEvent<HTMLSelectElement>) => {
    const nextValue = clampAutoDelay(Number(event.target.value));
    if (Number.isNaN(nextValue)) return;
    setAutoDelay(nextValue);
  }, []);

  const handleFallbackDelayChange = useCallback((event: ChangeEvent<HTMLSelectElement>) => {
    const nextValue = clampFallbackDelay(Number(event.target.value));
    if (Number.isNaN(nextValue)) return;
    setFallbackDelay(nextValue);
  }, []);

  useEffect(() => {
    if (introReady) {
      return;
    }

    if (typeof window === 'undefined') {
      setIntroReady(true);
      return;
    }

    const handleIntroDone = () => setIntroReady(true);
    window.addEventListener('vortex:intro:done', handleIntroDone);
    return () => {
      window.removeEventListener('vortex:intro:done', handleIntroDone);
    };
  }, [introReady]);

  useEffect(() => {
    if (!introReady) {
      return;
    }
    const nextStatus = lastProject ? `Welcome back, ${lastProject.name}` : 'Opening Hub…';
    setStatus(nextStatus);
    setHint(lastProject?.path ?? null);
    const computedFallback = resolveFallbackDelayForContext(fallbackDelay, Boolean(lastProject));
    if (!stickySplash) {
      scheduleHubRedirect(computedFallback);
    } else {
      clearFallback();
    }

    return () => {
      clearFallback();
      clearAutoTimer();
    };
  }, [clearAutoTimer, clearFallback, fallbackDelay, introReady, lastProject, scheduleHubRedirect, stickySplash]);

  const handleContinueLast = useCallback(async () => {
    if (!introReady) {
      return;
    }
    if (!lastProject?.path || isContinuing) {
      goToHub();
      return;
    }
    clearAutoTimer();
    setIsContinuing(true);
    setError(null);
    setStatus(`Opening ${lastProject.name}…`);
    setHint(lastProject.path);
    try {
      await loadProject(lastProject.path);
      clearFallback();
      nav('/editor');
    } catch (err) {
      console.error('[Splash] Failed to continue last project', err);
      setError(err instanceof Error ? err.message : 'Unable to open last project');
      setStatus('Unable to open last project');
      scheduleHubRedirect(5000);
    } finally {
      setIsContinuing(false);
    }
  }, [clearAutoTimer, clearFallback, goToHub, introReady, lastProject, loadProject, nav, scheduleHubRedirect, isContinuing]);

  const handleForgetLast = useCallback(() => {
    engine.clearLastProject();
    setLastProject(null);
    setHint(null);
    setError(null);
    setStatus('Opening Hub…');
    scheduleHubRedirect(800);
  }, [scheduleHubRedirect]);

  useEffect(() => {
    if (typeof window === 'undefined') return;
    window.localStorage.setItem(SPLASH_AUTO_CONTINUE_KEY, autoContinue ? 'true' : 'false');
  }, [autoContinue]);

  useEffect(() => {
    writeAutoDelayPreference(autoDelay);
  }, [autoDelay]);

  useEffect(() => {
    writeFallbackDelayPreference(fallbackDelay);
  }, [fallbackDelay]);

  useEffect(() => {
    writeSplashStickyPreference(stickySplash);
    if (stickySplash) {
      clearFallback();
    }
  }, [stickySplash, clearFallback]);

  useEffect(() => {
    if (!introReady || !autoContinue || !lastProject?.path || isContinuing) {
      clearAutoTimer();
      return;
    }

    autoTimerRef.current = window.setTimeout(() => {
      handleContinueLast();
    }, autoDelay);

    setAutoCountdown(Math.ceil(autoDelay / 1000));
    const startedAt = Date.now();
    countdownIntervalRef.current = window.setInterval(() => {
      const elapsed = Date.now() - startedAt;
      const remaining = Math.max(0, autoDelay - elapsed);
      setAutoCountdown(Math.max(0, Math.ceil(remaining / 1000)));
    }, 250);

    return () => {
      clearAutoTimer();
    };
  }, [autoContinue, autoDelay, clearAutoTimer, handleContinueLast, introReady, isContinuing, lastProject]);

  return (
    <div className="w-screen h-screen grid place-items-center bg-ui-bg">
      <motion.div
        initial={{ opacity: 0, scale: 0.9 }}
        animate={{ opacity: 1, scale: 1 }}
        transition={{ duration: 0.5 }}
        className="w-[480px] h-[320px] rounded-2xl shadow-xl border border-ui-border bg-ui-panel flex flex-col items-center justify-center gap-4"
      >
        <div className="text-2xl font-semibold">Vortex Studio</div>
        <div className="text-sm opacity-70 text-center px-6">{status}</div>
        {hint && <div className="text-xs text-gray-500 text-center px-6 break-all">{hint}</div>}
        <div className="flex flex-wrap justify-center gap-2">
          {lastProject?.path && (
            <button
              type="button"
              onClick={handleContinueLast}
              disabled={isContinuing}
              className={`rounded border px-3 py-1 text-sm transition ${
                isContinuing
                  ? 'border-ui-border text-gray-500 cursor-not-allowed'
                  : 'border-ui-accent/40 text-ui-accent hover:bg-ui-accent/10'
              }`}
            >
              {isContinuing ? 'Opening…' : 'Continue last project'}
            </button>
          )}
          <button
            type="button"
            onClick={goToHub}
            className="rounded border border-ui-border px-3 py-1 text-sm text-gray-100 hover:bg-white/5"
          >
            Open Hub now
          </button>
          {lastProject?.path && (
            <button
              type="button"
              onClick={handleForgetLast}
              className="rounded border border-red-500/30 px-3 py-1 text-xs text-red-200 hover:bg-red-500/10"
            >
              Forget last project
            </button>
          )}
        </div>
        {lastProject?.path && (
          <label className="flex items-center gap-2 text-xs text-gray-400">
            <input
              type="checkbox"
              checked={autoContinue}
              onChange={(event) => setAutoContinue(event.target.checked)}
              className="h-3.5 w-3.5 rounded border border-ui-border bg-black/40 text-ui-accent focus:ring-ui-accent" 
            />
            Auto-continue next time
          </label>
        )}
        <label className="flex items-center gap-2 text-xs text-gray-500">
          <input
            type="checkbox"
            checked={stickySplash}
            onChange={(event) => setStickySplash(event.target.checked)}
            className="h-3.5 w-3.5 rounded border border-ui-border bg-black/40 text-ui-accent focus:ring-ui-accent"
          />
          Keep this screen open
        </label>
        <div className="w-full px-6">
          <button
            type="button"
            onClick={() => setShowTiming((prev) => !prev)}
            className="flex w-full items-center justify-between rounded border border-ui-border/60 px-3 py-1.5 text-xs text-gray-400 transition hover:border-ui-accent/40 hover:text-gray-100"
            aria-expanded={showTiming}
          >
            <span>Timing preferences</span>
            <span className="text-[11px] text-gray-500">{timingSummary}</span>
          </button>
          {showTiming && (
            <div className="mt-3 space-y-3 rounded-lg border border-ui-border/60 bg-black/30 p-3 text-xs text-gray-300">
              <label className="flex flex-col gap-1">
                <span className="text-[11px] uppercase tracking-wide text-gray-500">Auto-continue delay</span>
                <select
                  value={autoDelay}
                  onChange={handleAutoDelayChange}
                  className="rounded border border-ui-border bg-black/40 px-2 py-1 text-gray-100 focus:border-ui-accent focus:outline-none"
                >
                  {autoDelayOptions.map((option) => (
                    <option key={option.value} value={option.value}>
                      {option.label}
                    </option>
                  ))}
                </select>
              </label>
              <label className="flex flex-col gap-1">
                <span className="text-[11px] uppercase tracking-wide text-gray-500">Splash fallback delay</span>
                <select
                  value={fallbackDelay}
                  onChange={handleFallbackDelayChange}
                  className="rounded border border-ui-border bg-black/40 px-2 py-1 text-gray-100 focus:border-ui-accent focus:outline-none"
                >
                  {fallbackDelayOptions.map((option) => (
                    <option key={option.value} value={option.value}>
                      {option.label}
                    </option>
                  ))}
                </select>
              </label>
            </div>
          )}
        </div>
        {lastProject?.path && autoContinue && autoCountdown != null && !isContinuing && (
          <div className="flex items-center gap-2 text-xs text-gray-400">
            <span>Auto-continue in {autoCountdown}s</span>
            <button
              type="button"
              onClick={() => {
                clearAutoTimer();
                setAutoContinue(false);
              }}
              className="rounded border border-ui-border/40 px-2 py-0.5 text-[11px] text-gray-200 transition hover:border-ui-border"
            >
              Cancel
            </button>
          </div>
        )}
        {error && <div className="text-xs text-red-300 text-center px-6">{error}</div>}
        <motion.div className="w-48 h-1 rounded bg-[#1f2430] overflow-hidden" initial={false}>
          <motion.div
            className="h-full bg-ui-accent"
            initial={{ x: '-100%' }}
            animate={{ x: ['-100%', '0%', '100%'] }}
            transition={{ repeat: Infinity, duration: 1.6, ease: 'easeInOut' }}
          />
        </motion.div>
      </motion.div>
    </div>
  );
}
