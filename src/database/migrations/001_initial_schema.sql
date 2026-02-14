-- Migration 001: Initial Schema
-- Creates all base tables for RootStream database layer

-- This migration is the base schema and should be run first
-- Run with: psql -U rootstream -d rootstream -f 001_initial_schema.sql

\i ../schema.sql
