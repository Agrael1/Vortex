import { useEffect, useRef, useState, type CSSProperties } from 'react';

const INTRO_DURATION = 7800;
const LETTER_STEP = 420;
const PARTICLES = 1600;
const SPIRAL_TURN = 1.34;
const SWIRL_STRENGTH = 0.04;
const DRIFT = 0.0009;
const CORE_RADIUS = 48;
const TRAIL = 0.08;
const COMETS = 9;
const LETTERS = 'VORTEX';

const INTRO_STYLES = `
.vortex-intro {
  position: fixed;
  inset: 0;
  z-index: 9999;
  pointer-events: none;
  isolation: isolate;
  --intro-bg: #06080c;
  --intro-fg: #e6f3ff;
  --intro-accent: #7dd3fc;
  --intro-duration: ${INTRO_DURATION}ms;
}
.vortex-intro::before {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(1200px 800px at 50% 55%, #0b1118 0%, #070b11 40%, var(--intro-bg) 72%, #04070b 100%) fixed;
}
.vortex-intro canvas {
  position: fixed;
  inset: 0;
  display: block;
  filter: contrast(105%) brightness(105%);
}
.vortex-intro__logo {
  position: fixed;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  display: flex;
  gap: 0.06em;
  pointer-events: none;
  user-select: none;
}
.vortex-intro__letter {
  position: relative;
  font-family: ui-sans-serif, system-ui, Inter, 'Segoe UI', Roboto, Arial, sans-serif;
  font-weight: 900;
  font-stretch: expanded;
  letter-spacing: 0.06em;
  font-size: clamp(48px, 12vw, 160px);
  line-height: 1;
  color: var(--intro-fg);
  text-shadow: 0 0 24px rgba(125, 211, 252, 0.35), 0 0 64px rgba(99, 102, 241, 0.25);
  opacity: 0;
  transform: translate3d(0, 8px, 0) scale(0.96);
  filter: blur(8px) saturate(110%);
}
.vortex-intro__letter::before,
.vortex-intro__letter::after {
  content: attr(data-char);
  position: absolute;
  inset: 0;
  color: var(--intro-fg);
  mix-blend-mode: screen;
  opacity: 0.25;
  filter: blur(2px);
}
.vortex-intro__letter::before {
  transform: translateX(-1px);
  color: #7dd3fc;
}
.vortex-intro__letter::after {
  transform: translateX(1px);
  color: #a78bfa;
}
.vortex-intro__letter.show {
  animation: vortex-pop-in 720ms cubic-bezier(0.2, 0.85, 0.1, 1) both;
}
@keyframes vortex-pop-in {
  0% {
    opacity: 0;
    transform: translate3d(0, 12px, 0) scale(0.9);
    filter: blur(10px) saturate(120%);
  }
  60% {
    opacity: 1;
    transform: translate3d(0, -2px, 0) scale(1.02);
    filter: blur(1.6px);
  }
  100% {
    opacity: 1;
    transform: translate3d(0, 0, 0) scale(1);
    filter: blur(0);
  }
}
.vortex-intro__logo.ready {
  animation: vortex-floaty 2600ms ease-in-out 1 both 5400ms;
}
@keyframes vortex-floaty {
  0% {
    transform: translate(-50%, -50%) scale(1);
  }
  50% {
    transform: translate(-50%, -50%) scale(1.02);
  }
  100% {
    transform: translate(-50%, -50%) scale(1);
  }
}
.vortex-intro__logo.fadeout {
  animation: vortex-fadeout var(--intro-duration) linear 1 forwards;
}
@keyframes vortex-fadeout {
  0%, 93% {
    opacity: 1;
  }
  100% {
    opacity: 0;
    visibility: hidden;
  }
}
`;

type IntroStyleProps = CSSProperties & Record<`--${string}`, string | number>;

const INTRO_STYLE_PROPS: IntroStyleProps = {
  '--intro-duration': `${INTRO_DURATION}ms`,
};

type Particle = {
  x: number;
  y: number;
  vx: number;
  vy: number;
  life: number;
  hue: number;
  alpha: number;
};

type Comet = {
  x: number;
  y: number;
  vx: number;
  vy: number;
  life: number;
};

export function IntroOverlay() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const logoRef = useRef<HTMLDivElement | null>(null);
  const [isDone, setIsDone] = useState(false);

  useEffect(() => {
    if (typeof document === 'undefined') {
      return;
    }

    const body = document.body;
    const html = document.documentElement;
    if (!body || !html) {
      return;
    }

    const prevBodyOverflow = body.style.overflow;
    const prevHtmlOverflow = html.style.overflow;

    if (!isDone) {
      body.style.overflow = 'hidden';
      html.style.overflow = 'hidden';
    }

    return () => {
      body.style.overflow = prevBodyOverflow;
      html.style.overflow = prevHtmlOverflow;
    };
  }, [isDone]);

  useEffect(() => {
    if (isDone) {
      return;
    }

    const isJsDom = typeof navigator !== 'undefined' && /jsdom/i.test(navigator.userAgent ?? '');
    if (isJsDom) {
      setIsDone(true);
      return;
    }

    const canvas = canvasRef.current;
    const logo = logoRef.current;
    if (!canvas || !logo) {
      return;
    }
    let ctx: CanvasRenderingContext2D | null = null;
    try {
      ctx = canvas.getContext('2d');
    } catch (error) {
      console.warn('[IntroOverlay] Unable to acquire 2d context', error);
    }
    if (!ctx) {
      setIsDone(true);
      return;
    }

    let animationFrame = 0;
    const timers: number[] = [];
    const DPR = Math.min(2, window.devicePixelRatio || 1);

    const resize = () => {
      canvas.width = window.innerWidth * DPR;
      canvas.height = window.innerHeight * DPR;
      ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
    };

    resize();
    window.addEventListener('resize', resize);

    const getCenter = () => ({ x: window.innerWidth / 2, y: window.innerHeight / 2 });

    const particles: Particle[] = Array.from({ length: PARTICLES }, () => ({
      x: 0,
      y: 0,
      vx: 0,
      vy: 0,
      life: 0,
      hue: 200 + Math.random() * 70,
      alpha: 0.25,
    }));

    const resetParticle = (p: Particle, hard = false) => {
      const { x: cx, y: cy } = getCenter();
      const angle = Math.random() * Math.PI * 2;
      const radius = CORE_RADIUS + Math.random() * 50;
      p.x = cx + Math.cos(angle) * radius;
      p.y = cy + Math.sin(angle) * radius;
      p.vx = 0;
      p.vy = 0;
      p.life = hard ? 0 : Math.random();
      p.hue = 200 + Math.random() * 70;
      p.alpha = 0.12 + Math.random() * 0.25;
    };

    particles.forEach((p) => resetParticle(p, true));

    const comets: Comet[] = Array.from({ length: COMETS }, () => ({
      x: 0,
      y: 0,
      vx: 0,
      vy: 0,
      life: -Math.random() * 2,
    }));

    const resetComet = (c: Comet) => {
      const { x: cx, y: cy } = getCenter();
      const angle = Math.random() * Math.PI * 2;
      const range = Math.min(window.innerWidth, window.innerHeight) * (0.35 + Math.random() * 0.25);
      const px = cx + Math.cos(angle) * range;
      const py = cy + Math.sin(angle) * range;
      const tx = -Math.sin(angle);
      const ty = Math.cos(angle);
      const speed = 3 + Math.random() * 2.5;
      c.x = px;
      c.y = py;
      c.vx = tx * speed;
      c.vy = ty * speed;
      c.life = 1.6 + Math.random() * 0.8;
    };

    const letterTimers: number[] = [];

    const buildLetters = () => {
      logo.innerHTML = '';
      logo.classList.remove('ready');
      [...LETTERS].forEach((char, index) => {
        const span = document.createElement('span');
        span.className = 'vortex-intro__letter';
        span.dataset.char = char;
        span.textContent = char;
        logo.appendChild(span);

        const timer = window.setTimeout(() => {
          const angle = (index / LETTERS.length) * Math.PI * 2 + Math.random() * 0.6 - 0.3;
          const dist = 6 + Math.random() * 10;
          span.style.transform = 'translate3d(0, 0, 0)';
          span.classList.add('show');
          span.animate(
            [
              { transform: `translate3d(${Math.cos(angle) * dist}px, ${Math.sin(angle) * dist}px, 0) scale(1.06)` },
              { transform: 'translate3d(0, 0, 0) scale(1)' },
            ],
            { duration: 680, easing: 'cubic-bezier(.2,.85,.1,1)', fill: 'forwards' },
          );
        }, index * LETTER_STEP);
        letterTimers.push(timer);
      });

      letterTimers.push(
        window.setTimeout(() => {
          logo.classList.add('ready');
        }, (LETTERS.length - 1) * LETTER_STEP + 450),
      );
    };

    buildLetters();

    let lastTime = performance.now();

    const step = (time: number) => {
      const dt = Math.min(32, time - lastTime);
      lastTime = time;

      ctx.globalCompositeOperation = 'source-over';
      ctx.fillStyle = `rgba(6, 8, 12, ${TRAIL})`;
      ctx.fillRect(0, 0, window.innerWidth, window.innerHeight);

      const { x: cx, y: cy } = getCenter();
      ctx.globalCompositeOperation = 'lighter';

      particles.forEach((p) => {
        const dx = p.x - cx;
        const dy = p.y - cy;
        const dist2 = dx * dx + dy * dy;
        const dist = Math.sqrt(dist2) + 1e-6;

        if (dist < CORE_RADIUS) {
          resetParticle(p);
          return;
        }

        const angle = Math.atan2(dy, dx) + SPIRAL_TURN * 0.002;
        const tx = -Math.sin(angle);
        const ty = Math.cos(angle);
        const pull = -0.0045 / (dist * 0.03);

        p.vx += tx * SWIRL_STRENGTH + dx * pull + (Math.random() - 0.5) * DRIFT;
        p.vy += ty * SWIRL_STRENGTH + dy * pull + (Math.random() - 0.5) * DRIFT;

        p.x += p.vx;
        p.y += p.vy;
        p.vx *= 0.985;
        p.vy *= 0.985;

        const alpha = p.alpha * (0.6 + Math.min(1, dist / (Math.min(window.innerWidth, window.innerHeight) * 0.8)));
        ctx.fillStyle = `hsla(${p.hue}, 95%, 72%, ${alpha})`;
        ctx.fillRect(p.x, p.y, 1.1, 1.1);

        if (
          p.x < -50 ||
          p.x > window.innerWidth + 50 ||
          p.y < -50 ||
          p.y > window.innerHeight + 50
        ) {
          resetParticle(p);
        }
      });

      comets.forEach((comet) => {
        if (comet.life <= 0) {
          resetComet(comet);
        }
        comet.life -= dt / 1000;
        comet.x += comet.vx;
        comet.y += comet.vy;
        ctx.strokeStyle = 'rgba(180, 220, 255, 0.35)';
        ctx.lineWidth = 1.2;
        ctx.beginPath();
        ctx.moveTo(comet.x, comet.y);
        ctx.lineTo(comet.x - comet.vx * 6, comet.y - comet.vy * 6);
        ctx.stroke();
      });

      animationFrame = window.requestAnimationFrame(step);
    };

    animationFrame = window.requestAnimationFrame((time) => {
      lastTime = time;
      step(time);
    });

    const finishTimer = window.setTimeout(() => {
      setIsDone(true);
    }, INTRO_DURATION);
    timers.push(finishTimer);

    return () => {
      window.removeEventListener('resize', resize);
      if (animationFrame) {
        window.cancelAnimationFrame(animationFrame);
      }
      letterTimers.forEach((id) => window.clearTimeout(id));
      timers.forEach((id) => window.clearTimeout(id));
    };
  }, [isDone]);

  if (isDone) {
    return null;
  }

  return (
    <div className="vortex-intro" style={INTRO_STYLE_PROPS} aria-hidden="true">
      <style>{INTRO_STYLES}</style>
      <canvas ref={canvasRef} />
      <div ref={logoRef} className="vortex-intro__logo fadeout" aria-label="VORTEX" />
    </div>
  );
}
