import { useCallback } from 'react';
import { useSetRecoilState } from 'recoil';
import { notificationsAtom, type NotificationLevel, type NotificationItem } from '@state/atoms/notifications';

const randomId = () => {
  if (typeof crypto !== 'undefined' && typeof crypto.randomUUID === 'function') {
    return crypto.randomUUID();
  }
  return `${Date.now()}-${Math.round(Math.random() * 1_000_000)}`;
};

export type PushNotificationPayload = {
  level: NotificationLevel;
  message: string;
  title?: string;
  scope?: string | null;
  durationMs?: number | null;
};

export function useNotificationCenter() {
  const setNotifications = useSetRecoilState(notificationsAtom);

  const push = useCallback((payload: PushNotificationPayload) => {
    const createdAt = Date.now();
    const expiresAt = typeof payload.durationMs === 'number' && payload.durationMs > 0 ? createdAt + payload.durationMs : null;
    const entry: NotificationItem = {
      id: randomId(),
      level: payload.level,
      title: payload.title,
      message: payload.message,
      createdAt,
      expiresAt,
      scope: payload.scope ?? null,
    };

    setNotifications((prev) => [...prev, entry]);
    return entry.id;
  }, [setNotifications]);

  const dismiss = useCallback((id: string) => {
    setNotifications((prev) => prev.filter((item) => item.id !== id));
  }, [setNotifications]);

  const clearAll = useCallback(() => {
    setNotifications([]);
  }, [setNotifications]);

  return { push, dismiss, clearAll };
}
