import { motion } from 'framer-motion'
import { useEffect } from 'react'
import { useNavigate } from 'react-router-dom'

export function Splash() {
  const nav = useNavigate()
  useEffect(() => {
    const t = setTimeout(() => nav('/hub'), 1200)
    return () => clearTimeout(t)
  }, [nav])

  return (
    <div className="w-screen h-screen grid place-items-center bg-ui-bg">
      <motion.div
        initial={{ opacity: 0, scale: 0.9 }}
        animate={{ opacity: 1, scale: 1 }}
        transition={{ duration: 0.5 }}
        className="w-[480px] h-[320px] rounded-2xl shadow-xl border border-ui-border bg-ui-panel flex flex-col items-center justify-center gap-4"
      >
        <div className="text-2xl font-semibold">Vortex Studio</div>
        <div className="text-sm opacity-70">Loading engine…</div>
        <motion.div
          className="w-48 h-1 rounded bg-[#1f2430] overflow-hidden"
          initial={false}
        >
          <motion.div
            className="h-full bg-ui-accent"
            initial={{ x: '-100%' }}
            animate={{ x: ['-100%','0%','100%'] }}
            transition={{ repeat: Infinity, duration: 1.6, ease: 'easeInOut' }}
          />
        </motion.div>
      </motion.div>
    </div>
  )
}