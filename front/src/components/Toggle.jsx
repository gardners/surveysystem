import React , { Component } from 'react';
import PropTypes from 'prop-types';

class Toggle extends Component{
    constructor(props) {
        super(props);
        this.state = {
            open: props.open,
        }
    }

    toggle(e) {
        e.preventDefault();
        this.setState({
            open: !this.state.open
        });
    }

    render() {
        const { open } = this.state;
        const [ header, ...contents ] = this.props.children;
        const cls = (!open) ? `d-print-block d-none` : '';

        return(
            <div className={ this.props.className }>
                <div role="menuitem" className={ this.props.classNameToggle } onClick={ this.toggle.bind(this) }>
                    { (!open) ? <i className="fas fa-caret-down d-inline-block d-print-none"></i> : <i className="fas fa-caret-up d-inline-block  d-print-none"></i> }<div className="d-inline-block ml-2">{ header }</div>
                </div>
                <div className={ cls }>{ contents }</div>
            </div>
        );
    }

};

Toggle.defaultProps = {
    open: false,
    classNameToggle: 'd-block text-primary',
};

Toggle.propTypes = {
    open: PropTypes.bool,
    className: PropTypes.string,
    classNameToggle: PropTypes.string,
};

export default Toggle;
