import { PROFILE_LINKS, PROJECT_LINKS } from '../lib/links.js'
import { cn, PageHeader } from '../components/ui.jsx'
import { BrandLogo } from '../components/BrandIcons.jsx'

const CHANNELS = [
  { id: 'email', label: 'Email', handle: 'kartykjdj@gmail.com', href: PROFILE_LINKS.email },
  { id: 'linkedin', label: 'LinkedIn', handle: 'kartick-kumar-ghosh', href: PROFILE_LINKS.linkedin },
  { id: 'github', label: 'GitHub', handle: '@gavinspet', href: PROFILE_LINKS.github },
  { id: 'instagram', label: 'Instagram', handle: '@bojack.fitman', href: PROFILE_LINKS.instagram },
]

function ChannelCard({ channel }) {
  const external = !channel.href.startsWith('mailto:')
  return (
    <li>
      <a
        className={cn('contact-card', `contact-card-${channel.id}`)}
        href={channel.href}
        {...(external ? { target: '_blank', rel: 'noopener noreferrer' } : {})}
      >
        <BrandLogo id={channel.id} />
        <span className="contact-card-text">
          <span className="contact-card-label">{channel.label}</span>
          <span className="contact-card-handle">{channel.handle}</span>
        </span>
        <span className="contact-card-go" aria-hidden="true">→</span>
        {external ? <span className="sr-only"> (opens in a new tab)</span> : null}
      </a>
    </li>
  )
}

export function ContactPage() {
  return (
    <section className="contact" aria-labelledby="contact-title">
      <PageHeader
        id="contact-title"
        eyebrow="Contact"
        title="Get in touch"
        description="Questions, feedback or collaboration ideas about Cortex — pick a channel."
      />

      <ul className="contact-grid">
        {CHANNELS.map((channel) => <ChannelCard key={channel.id} channel={channel} />)}
      </ul>

      <a className="contact-issue" href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">
        <BrandLogo id="github" />
        <span className="contact-card-text">
          <span className="contact-card-label">Found a bug?</span>
          <span className="contact-card-handle">Open an issue on Cortex-Code-Intelligence-Platform</span>
        </span>
        <span className="contact-card-go" aria-hidden="true">→</span>
        <span className="sr-only"> (opens in a new tab)</span>
      </a>
    </section>
  )
}
