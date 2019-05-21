import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';

import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import MenuIcon from '@material-ui/icons/Menu';
import Button from '@material-ui/core/Button';
import IconButton from '@material-ui/core/IconButton';
import WbSunny from '@material-ui/icons/WbSunny';
import WbSunnyOutlined from '@material-ui/icons/WbSunnyOutlined';

import HideOnScroll from './HideOnScroll';

class HeaderNav extends Component {
    constructor(props) {
        super(props);
        this.state = {
            anchorEl: null,
        }
    }

    handleMenu (e) {
        this.setState({
            anchorEl: (!this.state.anchorEl) ? e.currentTarget : null,
        });
    }

    render() {

        const  { surveys, siteName, themeType, toggleThemeType } = this.props;
        const  { anchorEl } = this.state;

        return (
            <HideOnScroll threshold={200 }>
                <AppBar position="relative">
                    <Toolbar>

                        <IconButton edge="start" color="inherit" aria-label="Menu">
                            <MenuIcon />
                        </IconButton>

                        <Button component={ Link } to="/" color="inherit">{ siteName }</Button>

                        {
                            surveys.length > 1 &&
                                <Button component={ Link } to="/surveys" color="inherit">Survey List</Button>
                        }

                        <Button
                            aria-owns={ anchorEl ? 'demo-menu' : undefined }
                            aria-haspopup="true"
                            onClick={ this.handleMenu.bind(this) }
                            color="inherit"
                        >
                            Demos
                        </Button>

                        <Menu id="demo-menu" anchorEl={ anchorEl } open={ anchorEl !== null } onClose={ this.handleMenu.bind(this) }>
                            <MenuItem onClick={ this.handleMenu.bind(this) } component={ Link } to="/demo/form">Form Elements</MenuItem>
                            <MenuItem onClick={ this.handleMenu.bind(this) } component={ Link } to="/demo/analyse">Analysis</MenuItem>
                            <MenuItem onClick={ this.handleMenu.bind(this) } component={ Link } to="/demo/manifest">Manifest</MenuItem>
                        </Menu>

                        <IconButton
                            color="inherit"
                            onClick={ toggleThemeType }
                        >
                            { (themeType === 'light') ? <WbSunnyOutlined /> : <WbSunny /> }
                        </IconButton>
                    </Toolbar>
                </AppBar>
            </HideOnScroll>
        );
    }
};

HeaderNav.propTypes = {
    surveys: PropTypes.arrayOf(PropTypes.string).isRequired,
    surveyProvider: PropTypes.string.isRequired,
    siteName: PropTypes.string.isRequired,
    themeType: PropTypes.oneOf(['light', 'dark' ]).isRequired,
};

export default HeaderNav;
