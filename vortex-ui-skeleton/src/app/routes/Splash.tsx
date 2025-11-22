import { motion } from 'framer-motion';
import { useCallback, useEffect, useRef, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { engine, type LastProject } from '@/app/services/ipc/cefBridge';
import { useProjectCommands } from '@state/hooks/useProjectCommands';
import { SPLASH_AUTO_CONTINUE_KEY } from '@/app/constants/preferences';

export function Splash() {
  const nav = useNavigate();
  const { loadProject } = useProjectCommands();
  const [lastProject, setLastProject] = useState<LastProject | null>(() => engine.getLastProject());
  const [status, setStatus] = useState(() => (lastProject ? `Welcome back, ${lastProject.name}` : 'Initializing workspace…'));
  const [hint, setHint] = useState<string | null>(() => lastProject?.path ?? null);
  const [error, setError] = useState<string | null>(null);
  const [isContinuing, setIsContinuing] = useState(false);
  const [autoContinue, setAutoContinue] = useState(() => {
    if (typeof window === 'undefined') return false;
    return window.localStorage.getItem(SPLASH_AUTO_CONTINUE_KEY) === 'true';
  });
  const fallbackTimerRef = useRef<number | null>(null);
  const autoTimerRef = useRef<number | null>(null);

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

  useEffect(() => {
    const nextStatus = lastProject ? `Welcome back, ${lastProject.name}` : 'Opening Hub…';
    setStatus(nextStatus);
    setHint(lastProject?.path ?? null);
    scheduleHubRedirect(lastProject ? 3000 : 1200);

    return () => {
      clearFallback();
      clearAutoTimer();
    };
  }, [clearAutoTimer, clearFallback, lastProject, scheduleHubRedirect]);

  const handleContinueLast = useCallback(async () => {
    if (!lastProject?.path || isContinuing) {
      goToHub();
      return;
    }
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
  }, [clearFallback, goToHub, lastProject, loadProject, nav, scheduleHubRedirect, isContinuing]);

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
    if (!autoContinue || !lastProject?.path || isContinuing) {
      clearAutoTimer();
      return;
    }

    autoTimerRef.current = window.setTimeout(() => {
      handleContinueLast();
    }, 1200);

    return () => {
      clearAutoTimer();
    };
  }, [autoContinue, clearAutoTimer, handleContinueLast, isContinuing, lastProject]);

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
