import { atom } from 'recoil';

export type NotificationLevel = 'info' | 'success' | 'warn' | 'error';

export type NotificationItem = {
  id: string;
  level: NotificationLevel;
  title?: string;
  message: string;
  createdAt: number;
  expiresAt?: number | null;
  scope?: string | null;
};

export const notificationsAtom = atom<NotificationItem[]>({
  key: 'notifications:list',
  default: [],
});
