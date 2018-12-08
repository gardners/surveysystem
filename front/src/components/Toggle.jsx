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
        console.log('toggle');
        e.preventDefault();
        this.setState({
            open: !this.state.open
        });
    }

    render() {

        const { open } = this.state;
        const [ header, ...contents ] = this.props.children;
        return(
            <div className={ this.props.className }>
                <a className="d-block text-primary" onClick={ this.toggle.bind(this) }>
                    { (!open) ? <i className="fas fa-plus-square d-inline-block"></i> : <i className="fas fa-minus-square d-inline-block"></i> }<div className="d-inline-block ml-2">{ header }</div>
                </a>
                { open && contents }
            </div>
        );
    }

};

Toggle.defaultProps = {
    open: false,

};

Toggle.propTypes = {
    open: PropTypes.bool,
    className: PropTypes.string,
};

export default Toggle;
