import React , { Component } from 'react';
import PropTypes from 'prop-types';
import Icon from '@material-ui/core/Icon';

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
        const { classes } = this.props;
        const [ header, ...contents ] = this.props.children;

        return(
            <div className={ this.props.className }>
                <div role="menuitem"  onClick={ this.toggle.bind(this) }>
                    <Icon color="primary">{ (!open) ? 'keyboard_arrow_down' : 'keyboard_arrow_down' }</Icon> <React.Fragment>{ header }</React.Fragment>
                </div>
                { open && <div>{ contents }</div> }
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
