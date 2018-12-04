import React from 'react'
import PropTypes from 'prop-types'

// *******************************************************
// HANDLE COMPONENT
// *******************************************************
export function Handle({
  divOrButton: Comp,
  domain: [min, max],
  handle: { id, value, percent },
  getHandleProps,
}) {
  return (
    <Comp
      role="slider"
      aria-valuemin={min}
      aria-valuemax={max}
      aria-valuenow={value}
      style={{
        left: `${percent}%`,
        position: 'absolute',
        marginLeft: '-11px',
        marginTop: '-6px',
        zIndex: 2,
        width: 24,
        height: 24,
        cursor: 'pointer',
        border: 0,
        borderRadius: '50%',
        boxShadow: '1px 1px 1px 1px rgba(0, 0, 0, 0.2)',
        backgroundColor: '#34568f',
      }}
      {...getHandleProps(id)}
    />
  )
}

Handle.propTypes = {
  divOrButton: PropTypes.oneOf(['div', 'button']).isRequired, // button allows keyboard events
  domain: PropTypes.array.isRequired,
  handle: PropTypes.shape({
    id: PropTypes.string.isRequired,
    value: PropTypes.number.isRequired,
    percent: PropTypes.number.isRequired,
  }).isRequired,
  getHandleProps: PropTypes.func.isRequired,
}

Handle.defaultProps = {
  divOrButton: 'div',
}

// *******************************************************
// TRACK COMPONENT
// *******************************************************
export function Track({ source, target, getTrackProps }) {
  return (
    <div
      style={{
        position: 'absolute',
        height: 14,
        zIndex: 1,
        backgroundColor: '#7aa0c4',
        borderRadius: 7,
        cursor: 'pointer',
        left: `${source.percent}%`,
        width: `${target.percent - source.percent}%`,
      }}
      {...getTrackProps()}
    />
  )
}

Track.propTypes = {
  source: PropTypes.shape({
    id: PropTypes.string.isRequired,
    value: PropTypes.number.isRequired,
    percent: PropTypes.number.isRequired,
  }).isRequired,
  target: PropTypes.shape({
    id: PropTypes.string.isRequired,
    value: PropTypes.number.isRequired,
    percent: PropTypes.number.isRequired,
  }).isRequired,
  getTrackProps: PropTypes.func.isRequired,
}

// *******************************************************
// TICK COMPONENT
// *******************************************************
export function Tick({ tick, count, format }) {
    const type = (tick.percent > 0 && tick.percent < 100) ? 'sun' : 'moon';
    return (
        <i
        className={ `fas fa-${type}` }
        style={{
          position: 'absolute',
          marginTop: 14,
          width: '1em',
          height: '1em',
          left: `${tick.percent}%`,
        }}
      ></i>
  )
}

Tick.propTypes = {
  tick: PropTypes.shape({
    id: PropTypes.string.isRequired,
    value: PropTypes.number.isRequired,
    percent: PropTypes.number.isRequired,
  }).isRequired,
  count: PropTypes.number.isRequired,
  format: PropTypes.func.isRequired,
}

Tick.defaultProps = {
  format: d => d,
}
