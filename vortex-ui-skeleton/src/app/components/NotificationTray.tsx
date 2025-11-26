import { useEffect, useMemo } from 'react';
import { useRecoilValue } from 'recoil';
import { notificationsAtom } from '@state/atoms/notifications';
import { useNotificationCenter } from '@state/hooks/useNotificationCenter';

const levelStyles: Record<string, string> = {
  info: 'border-white/10 bg-black/70 text-gray-100',
  success: 'border-emerald-400/30 bg-emerald-900/40 text-emerald-50',
  warn: 'border-amber-300/40 bg-amber-900/30 text-amber-50',
  error: 'border-red-400/50 bg-red-900/50 text-red-50',
};

export function NotificationTray() {
  const notifications = useRecoilValue(notificationsAtom);
  const { dismiss } = useNotificationCenter();

  useEffect(() => {
    const timers = notifications.map((note) => {
      if (!note.expiresAt) {
        return null;
      }
      const delay = note.expiresAt - Date.now();
      if (delay <= 0) {
        dismiss(note.id);
        return null;
      }
      const timeoutId = window.setTimeout(() => dismiss(note.id), delay);
      return () => clearTimeout(timeoutId);
    });

    return () => {
      timers.forEach((dispose) => dispose?.());
    };
  }, [dismiss, notifications]);

  const ordered = useMemo(() => notifications.slice().sort((a, b) => a.createdAt - b.createdAt), [notifications]);

  if (ordered.length === 0) {
    return null;
  }

  return (
    <div className="pointer-events-none fixed bottom-4 right-4 z-50 flex w-full max-w-sm flex-col gap-3 px-4 sm:px-0">
      {ordered.map((note) => (
        <div
          key={note.id}
          className={`pointer-events-auto rounded-2xl border px-4 py-3 shadow-2xl backdrop-blur ${levelStyles[note.level] ?? levelStyles.info}`}
        >
          <div className="flex items-start gap-3">
            <div className="flex-1 space-y-1">
              <div className="text-sm font-semibold">
                {note.title ?? (note.level === 'error' ? 'Error' : 'Notification')}
              </div>
              <div className="text-xs text-gray-200 whitespace-pre-line">
                {note.message}
              </div>
              {note.scope ? <div className="text-[11px] uppercase tracking-wide text-gray-400">{note.scope}</div> : null}
            </div>
            <button
              type="button"
              aria-label="Dismiss notification"
              className="text-gray-400 transition hover:text-white"
              onClick={() => dismiss(note.id)}
            >
              ✕
            </button>
          </div>
        </div>
      ))}
    </div>
  );
}
