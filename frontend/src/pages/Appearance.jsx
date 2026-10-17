import { cn, PageHeader } from '../components/ui.jsx'

const OPTIONS = [
  { id: 'light', label: 'Light', description: 'White surfaces with blue accents. Clear in bright rooms.' },
  { id: 'dark', label: 'Dark', description: 'Deep navy surfaces that are easier on the eyes at night.' },
]

// A miniature Cortex page drawn in the option's own palette, independent of the active theme.
function ThemePreview({ id }) {
  return (
    <span className={cn('theme-preview', `theme-preview-${id}`)} aria-hidden="true">
      <span className="theme-preview-nav">
        <span className="theme-preview-logo" />
        <span className="theme-preview-line is-short" />
        <span className="theme-preview-pill" />
      </span>
      <span className="theme-preview-hero">
        <span className="theme-preview-line is-hero" />
        <span className="theme-preview-line is-hero is-short" />
      </span>
      <span className="theme-preview-cards">
        <span className="theme-preview-card">
          <span className="theme-preview-ring" />
          <span className="theme-preview-line is-muted" />
        </span>
        <span className="theme-preview-card">
          <span className="theme-preview-bars"><span /><span /><span /></span>
          <span className="theme-preview-line is-muted" />
        </span>
      </span>
    </span>
  )
}

export function AppearancePage({ theme, setTheme }) {
  return (
    <section className="appearance" aria-labelledby="appearance-title">
      <PageHeader
        id="appearance-title"
        eyebrow="Settings"
        title="Appearance"
        description="Choose how Cortex looks. Your choice is saved in this browser."
      />

      <fieldset className="appearance-options">
        <legend className="appearance-legend">Theme</legend>
        {OPTIONS.map((option, index) => {
          const selected = theme === option.id
          return (
            <label key={option.id} className={cn('appearance-option', selected && 'is-selected')} style={{ '--i': index }}>
              <input
                className="sr-only"
                type="radio"
                name="theme"
                value={option.id}
                checked={selected}
                onChange={() => setTheme(option.id)}
              />
              <ThemePreview id={option.id} />
              <span className="appearance-option-body">
                <span className="appearance-option-head">
                  <span className="appearance-radio" aria-hidden="true" />
                  <strong>{option.label}</strong>
                  {selected ? <span className="appearance-current">Active</span> : null}
                </span>
                <span className="appearance-option-text">{option.description}</span>
              </span>
            </label>
          )
        })}
      </fieldset>

      <p className="appearance-tip">Tip: the sun and moon button in the top bar switches themes from any page.</p>
    </section>
  )
}
