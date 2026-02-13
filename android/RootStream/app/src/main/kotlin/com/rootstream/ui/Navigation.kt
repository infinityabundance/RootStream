package com.rootstream.ui

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.rootstream.ui.screens.LoginScreen
import com.rootstream.ui.screens.PeerDiscoveryScreen
import com.rootstream.ui.screens.SettingsScreen
import com.rootstream.ui.screens.StreamScreen

/**
 * Navigation routes for the application
 */
sealed class Screen(val route: String) {
    object Login : Screen("login")
    object Discovery : Screen("discovery")
    object Stream : Screen("stream/{peerId}") {
        fun createRoute(peerId: String) = "stream/$peerId"
    }
    object Settings : Screen("settings")
}

/**
 * Main navigation composable
 * Sets up the navigation graph with all screens
 */
@Composable
fun MainNavigation() {
    val navController = rememberNavController()
    
    NavHost(
        navController = navController,
        startDestination = Screen.Login.route
    ) {
        composable(Screen.Login.route) {
            LoginScreen(
                onLoginSuccess = {
                    navController.navigate(Screen.Discovery.route) {
                        popUpTo(Screen.Login.route) { inclusive = true }
                    }
                }
            )
        }
        
        composable(Screen.Discovery.route) {
            PeerDiscoveryScreen(
                onPeerSelected = { peerId ->
                    navController.navigate(Screen.Stream.createRoute(peerId))
                },
                onNavigateToSettings = {
                    navController.navigate(Screen.Settings.route)
                }
            )
        }
        
        composable(Screen.Stream.route) { backStackEntry ->
            val peerId = backStackEntry.arguments?.getString("peerId") ?: ""
            StreamScreen(
                peerId = peerId,
                onDisconnect = {
                    navController.popBackStack()
                }
            )
        }
        
        composable(Screen.Settings.route) {
            SettingsScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }
    }
}
