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
        const { titleTag: Title, title, className, titleClassName, children } = this.props;
        const pClass = (!open) ? `d-print-block d-none` : 'd-print-block';
        const iClass = (!open) ? 'fas fa-caret-down' : 'fas fa-caret-up';

        return(
            <div className={ className }>
                <Title className={ titleClassName } role="menuitem" onClick={ this.toggle.bind(this) }>
                    <i className={ iClass }/> { title }
                </Title>
                <div className={ pClass }>{ children }</div>
            </div>
        );
    }

};

Toggle.defaultProps = {
    open: false,
    className: '',
    titleClassName: 'text-primary',
    titleTag: 'div',
};

Toggle.propTypes = {
    title: PropTypes.string.isRequired,
    titleTag: PropTypes.string,
    open: PropTypes.bool,
    className: PropTypes.string,
    titleClassName: PropTypes.string,
};

export default Toggle;
